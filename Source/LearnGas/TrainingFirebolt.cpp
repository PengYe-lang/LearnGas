#include "TrainingFirebolt.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "CombatAttributes.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	constexpr float AimRange = 2400.f;
	constexpr float BoltSpeed = 1600.f;
}

void ATrainingCrosshairTarget::ConfirmTargetingAndContinue()
{
	if (!PrimaryPC || !OwningAbility)
	{
		CanceledDelegate.Broadcast(FGameplayAbilityTargetDataHandle());
		return;
	}
	FVector Eye;
	FRotator View;
	PrimaryPC->GetPlayerViewPoint(Eye, View);
	const FVector End = Eye + View.Vector() * AimRange;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(TrainingCrosshair), false, OwningAbility->GetAvatarActorFromActorInfo());
	FHitResult Wall, Pawn;
	const bool bWall = GetWorld()->LineTraceSingleByChannel(Wall, Eye, End, ECC_Visibility, Query);
	FCollisionObjectQueryParams Characters(ECC_Pawn);
	const bool bPawn = GetWorld()->LineTraceSingleByObjectType(Pawn, Eye, bWall ? Wall.ImpactPoint : End, Characters, Query);
	auto* Aim = new FGameplayAbilityTargetData_LocationInfo();
	Aim->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	Aim->TargetLocation.LiteralTransform.SetLocation(bPawn ? Pawn.ImpactPoint : bWall ? Wall.ImpactPoint : End);
	TargetDataReadyDelegate.Broadcast(FGameplayAbilityTargetDataHandle(Aim));
}

UTrainingFireboltHit::UTrainingFireboltHit()
{
	BaseDamage = 24.f;
	bFireDamage = true;
}

UTrainingFireboltCost::UTrainingFireboltCost()
{
	auto& Cost = Modifiers.AddDefaulted_GetRef();
	Cost.Attribute = UCombatAttributes::GetStaminaAttribute();
	Cost.ModifierOp = EGameplayModOp::Additive;
	Cost.ModifierMagnitude = FScalableFloat(-18.f);
}

ATrainingFirebolt::ATrainingFirebolt()
{
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = AimRange / BoltSpeed;
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Sphere);
	Sphere->InitSphereRadius(9.f);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionObjectType(ECC_WorldDynamic);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Sphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	auto* Shell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ember"));
	Shell->SetupAttachment(Sphere);
	Shell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Shell->SetRelativeScale3D(FVector(.23f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Ball(TEXT("/Engine/BasicShapes/Sphere"));
	Shell->SetStaticMesh(Ball.Object);
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flight"));
	Movement->UpdatedComponent = Sphere;
	Movement->InitialSpeed = Movement->MaxSpeed = BoltSpeed;
	Movement->Velocity = FVector::ForwardVector * BoltSpeed;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = true;
}

void ATrainingFirebolt::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	Sphere->IgnoreActorWhenMoving(GetInstigator(), true);
	if (HasAuthority()) Movement->OnProjectileStop.AddDynamic(this, &ThisClass::Impact);
	else Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (auto* Mesh = FindComponentByClass<UStaticMeshComponent>())
		if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, .18f, .015f));
}

void ATrainingFirebolt::Impact(const FHitResult& Hit)
{
	if (!HasAuthority() || IsActorBeingDestroyed()) return;
	auto* Target = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Hit.GetActor());
	if (Target && Damage.IsValid())
	{
		Damage.Data->GetContext().AddHitResult(Hit);
		Target->ApplyGameplayEffectSpecToSelf(*Damage.Data);
	}
	Destroy();
}

UTrainingFireboltAbility::UTrainingFireboltAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CostGameplayEffectClass = UTrainingFireboltCost::StaticClass();
	ActivationBlockedTags.AddTag(TrainingDodging);
	ActivationBlockedTags.AddTag(TrainingHitStun);
	ActivationBlockedTags.AddTag(TrainingAttack);
}

void UTrainingFireboltAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	auto* Timeout = UAbilityTask_WaitDelay::WaitDelay(this, 2.f);
	Timeout->OnFinish.AddDynamic(this, &ThisClass::AimExpired);
	Timeout->ReadyForActivation();
	const auto Class = ATrainingCrosshairTarget::StaticClass();
	auto* Task = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::Instant, Class);
	Task->ValidData.AddDynamic(this, &ThisClass::Fire);
	Task->Cancelled.AddDynamic(this, &ThisClass::CancelAim);
	Task->ReadyForActivation();
	AGameplayAbilityTargetActor* Target = nullptr;
	if (Task->BeginSpawningActor(this, Class, Target)) Task->FinishSpawningActor(this, Target);
}

void UTrainingFireboltAbility::Fire(const FGameplayAbilityTargetDataHandle& Data)
{
	const auto* Info = GetCurrentActorInfo();
	auto* Pawn = Cast<APawn>(Info->AvatarActor.Get());
	const auto* Aim = Data.Num() == 1 ? Data.Get(0) : nullptr;
	if (!Pawn || !Aim || Aim->GetScriptStruct() != FGameplayAbilityTargetData_LocationInfo::StaticStruct())
	{
		AimExpired();
		return;
	}
	const auto* Location = static_cast<const FGameplayAbilityTargetData_LocationInfo*>(Aim);
	if (Location->TargetLocation.LocationType != EGameplayAbilityTargetingLocationType::LiteralTransform)
	{
		AimExpired();
		return;
	}
	const FVector End = Location->TargetLocation.LiteralTransform.GetLocation();
	const FVector Start = Pawn->GetActorLocation() + FVector(0, 0, 34);
	const FVector Ray = End - Start;
	const FVector View = Pawn->GetBaseAimRotation().Vector();
	if (End.ContainsNaN() || Ray.IsNearlyZero() || Ray.SizeSquared() > FMath::Square(AimRange + 400.f)
		|| FVector::DotProduct(View, Ray.GetSafeNormal()) < .35f || !CommitAbility(CurrentSpecHandle, Info, CurrentActivationInfo))
	{
		AimExpired();
		return;
	}
	if (Info->IsNetAuthority())
	{
		const FTransform Launch(Ray.Rotation(), Start);
		auto* Bolt = GetWorld()->SpawnActorDeferred<ATrainingFirebolt>(ATrainingFirebolt::StaticClass(),
			Launch, Pawn, Pawn, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Bolt)
		{
			Bolt->Damage = MakeOutgoingGameplayEffectSpec(UTrainingFireboltHit::StaticClass(), GetAbilityLevel());
			Bolt->FinishSpawning(Launch);
		}
	}
	EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, Info->IsNetAuthority(), false);
}

void UTrainingFireboltAbility::CancelAim(const FGameplayAbilityTargetDataHandle& Data)
{
	AimExpired();
}

void UTrainingFireboltAbility::AimExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
