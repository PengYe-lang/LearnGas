#include "TrainingSentinel.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "CombatAttributes.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "TrainingCharacter.h"
#include "TrainingDefeatReward.h"
#include "TimerManager.h"

namespace
{
	constexpr float Leash = 780.f;
	constexpr float NoticeRange = 520.f;
	constexpr float StrikeReach = 155.f;
}

ATrainingSentinel::ATrainingSentinel()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = 240.f;
	GetCapsuleComponent()->InitCapsuleSize(38,90);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Body(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Combat/ABP_TrainingCombat"));
	GetMesh()->SetSkeletalMesh(Body.Object);
	GetMesh()->SetAnimInstanceClass(Anim.Class);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-90), FRotator(0,-90,0));
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributes = CreateDefaultSubobject<UCombatAttributes>(TEXT("Attributes"));
	CreateDefaultSubobject<UTrainingDefeatReward>(TEXT("DefeatReward"));
}

void ATrainingSentinel::BeginPlay()
{
	Super::BeginPlay();
	Home = GetActorLocation();
	ASC->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		ASC->GiveAbility(FGameplayAbilitySpec(UTrainingSentinelAttack::StaticClass(), 1));
		ASC->GetGameplayAttributeValueChangeDelegate(UCombatAttributes::GetHealthAttribute()).AddUObject(this, &ThisClass::HealthChanged);
	}
}

void ATrainingSentinel::SetChallenger(ATrainingCharacter* Player)
{
	bInEncounter = true;
	Challenger = Player;
}

void ATrainingSentinel::Tick(float Delta)
{
	Super::Tick(Delta);
	if (!HasAuthority() || Phase != ETrainingSentinelPhase::Ready) return;
	float Best = FMath::Square(bInEncounter ? 1400.f : NoticeRange);
	Target.Reset();
	for (TActorIterator<ATrainingCharacter> It(GetWorld()); It; ++It)
	{
		if (bInEncounter && *It != Challenger.Get()) continue;
		if (!bInEncounter && FVector::DistSquared(Home, It->GetActorLocation()) > FMath::Square(Leash)) continue;
		const auto* PlayerASC = It->GetAbilitySystemComponent();
		if (!PlayerASC || PlayerASC->GetNumericAttribute(UCombatAttributes::GetHealthAttribute()) <= 0) continue;
		const float Distance = FVector::DistSquared(It->GetActorLocation(), GetActorLocation());
		if (Distance < Best)
		{
			Best = Distance;
			Target = *It;
		}
	}
	const FVector Destination = Target.IsValid() ? Target->GetActorLocation() : Home;
	const FVector Direction = Destination - GetActorLocation();
	if (!Target.IsValid() || Best > FMath::Square(115.f))
	{
		if (Direction.SizeSquared2D() > FMath::Square(12.f)) AddMovementInput(Direction.GetSafeNormal2D(), 1.f);
		return;
	}
	GetCharacterMovement()->StopMovementImmediately();
	if (GetWorld()->GetTimeSeconds() >= NextAttackTime)
	{
		SetActorRotation(Direction.GetSafeNormal2D().Rotation());
		ASC->TryActivateAbilityByClass(UTrainingSentinelAttack::StaticClass());
	}
}

void ATrainingSentinel::SetPhase(ETrainingSentinelPhase NewPhase)
{
	Phase = NewPhase;
	OnRep_Phase();
	ForceNetUpdate();
}

void ATrainingSentinel::OnRep_Phase()
{
	const bool bDown = Phase == ETrainingSentinelPhase::Down;
	GetMesh()->SetVisibility(!bDown, true);
	GetCapsuleComponent()->SetCollisionEnabled(bDown ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
}

void ATrainingSentinel::FinishAttack()
{
	if (Phase == ETrainingSentinelPhase::Down) return;
	SetPhase(ETrainingSentinelPhase::Ready);
	NextAttackTime = GetWorld()->GetTimeSeconds() + 1.20;
}

void ATrainingSentinel::HealthChanged(const FOnAttributeChangeData& Change)
{
	if (Change.OldValue <= 0 || Change.NewValue > 0) return;
	SetPhase(ETrainingSentinelPhase::Down);
	ASC->CancelAbilities();
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TrainingBurning));
	GetCharacterMovement()->DisableMovement();
	if (!bInEncounter) GetWorldTimerManager().SetTimer(ResetTimer, this, &ThisClass::ResetOpponent, 5.f);
}

void ATrainingSentinel::ResetOpponent()
{
	SetActorLocation(Home, false, nullptr, ETeleportType::TeleportPhysics);
	ASC->SetNumericAttributeBase(UCombatAttributes::GetHealthAttribute(), Attributes->GetMaxHealth());
	SetPhase(ETrainingSentinelPhase::Ready);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	NextAttackTime = GetWorld()->GetTimeSeconds() + 1.;
}

void ATrainingSentinel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrainingSentinel, Phase);
}

UTrainingSentinelDamage::UTrainingSentinelDamage()
{
	BaseDamage = 16.f;
}

UTrainingSentinelAttack::UTrainingSentinelAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AttackMontage = FSoftObjectPath(TEXT("/Game/Combat/AM_LightStrike.AM_LightStrike"));
}

void UTrainingSentinelAttack::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	auto* Enemy = CastChecked<ATrainingSentinel>(Info->AvatarActor.Get());
	Victim = Enemy->GetTarget();
	if (!Victim.IsValid() || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, false, true);
		return;
	}
	Enemy->SetPhase(ETrainingSentinelPhase::Windup);
	auto* Delay = UAbilityTask_WaitDelay::WaitDelay(this, .55f);
	Delay->OnFinish.AddDynamic(this, &ThisClass::Swing);
	Delay->ReadyForActivation();
}

void UTrainingSentinelAttack::Swing()
{
	auto* Enemy = CastChecked<ATrainingSentinel>(GetAvatarActorFromActorInfo());
	Enemy->SetPhase(ETrainingSentinelPhase::Swing);
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	if (auto* Montage = AttackMontage.LoadSynchronous())
	{
		ASC->PlayMontage(this, CurrentActivationInfo, Montage, 1.f, TEXT("Light1"));
		ASC->CurrentMontageSetNextSectionName(TEXT("Light1"), NAME_None);
	}
	auto* Delay = UAbilityTask_WaitDelay::WaitDelay(this, .30f);
	Delay->OnFinish.AddDynamic(this, &ThisClass::ResolveHit);
	Delay->ReadyForActivation();
}

void UTrainingSentinelAttack::ResolveHit()
{
	auto* Enemy = GetAvatarActorFromActorInfo();
	if (Victim.IsValid())
	{
		const FVector Delta = Victim->GetActorLocation() - Enemy->GetActorLocation();
		const bool bInArc = FVector::DotProduct(Enemy->GetActorForwardVector(), Delta.GetSafeNormal2D()) > .5f;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(SentinelStrike), false, Enemy);
		Query.AddIgnoredActor(Victim.Get());
		if (Delta.SizeSquared() <= FMath::Square(StrikeReach) && bInArc
			&& !GetWorld()->LineTraceTestByChannel(Enemy->GetActorLocation(), Victim->GetActorLocation(), ECC_Visibility, Query))
		{
			const auto Damage = MakeOutgoingGameplayEffectSpec(UTrainingSentinelDamage::StaticClass(), 1);
			if (auto* TargetASC = Victim->GetAbilitySystemComponent(); TargetASC && Damage.IsValid())
				GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Damage.Data, TargetASC);
		}
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UTrainingSentinelAttack::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled)
{
	if (bCancelled) Info->AbilitySystemComponent->CurrentMontageStop(.1f);
	if (auto* Enemy = Cast<ATrainingSentinel>(Info->AvatarActor.Get())) Enemy->FinishAttack();
	Victim.Reset();
	Super::EndAbility(Handle, Info, Activation, bReplicate, bCancelled);
}
