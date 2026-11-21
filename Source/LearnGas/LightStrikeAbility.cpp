#include "LightStrikeAbility.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimMontage.h"
#include "CombatAttributes.h"
#include "TrainingCharacter.h"
#include "Engine/World.h"

ULightStrikeAbility::ULightStrikeAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AttackMontage = FSoftObjectPath(TEXT("/Game/Combat/AM_LightStrike.AM_LightStrike"));
}

void ULightStrikeAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	Super::ActivateAbility(Handle, Info, Activation, Event);
	auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get());
	auto* Montage = AttackMontage.LoadSynchronous();
	if (!Character || !Montage || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, true, true);
		return;
	}

	HitActors.Reset();
	Character->ActiveStrike = this;
	auto* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	Play->OnCompleted.AddDynamic(this, &ThisClass::FinishStrike);
	Play->OnInterrupted.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->OnCancelled.AddDynamic(this, &ThisClass::InterruptStrike);
	Play->ReadyForActivation();
}

void ULightStrikeAbility::SampleHand(bool bFirstSample)
{
	auto* Character = Cast<ATrainingCharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->HasAuthority()) return;

	static const FName HandSocket(TEXT("hand_r"));
	const FVector Hand = Character->GetMesh()->GetSocketLocation(HandSocket);
	const FVector Start = bFirstSample ? Hand : PreviousHand;
	PreviousHand = Hand;
	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(LightStrike), false, Character);
	GetWorld()->SweepMultiByObjectType(Hits, Start, Hand, FQuat::Identity, Objects, FCollisionShape::MakeSphere(22.f), Query);

	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC || HitActors.Contains(Target)) continue;
		const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
		if (FVector::DotProduct(Character->GetActorForwardVector(), ToTarget) <= 0.f) continue;

		auto Damage = MakeOutgoingGameplayEffectSpec(ULightStrikeDamage::StaticClass(), GetAbilityLevel());
		if (Damage.IsValid())
		{
			HitActors.Add(Target);
			Damage.Data->GetContext().AddHitResult(Hit);
			GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*Damage.Data, TargetASC);
		}
	}
}

void ULightStrikeAbility::FinishStrike()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void ULightStrikeAbility::InterruptStrike()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void ULightStrikeAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled)
{
	if (auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get()); Character && Character->ActiveStrike.Get() == this)
	{
		Character->ActiveStrike.Reset();
	}
	HitActors.Reset();
	Super::EndAbility(Handle, Info, Activation, bReplicate, bCancelled);
}

void ULightStrikeWindow::NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(Mesh, Animation, Duration, EventReference);
	if (auto* Character = Cast<ATrainingCharacter>(Mesh->GetOwner()); Character && Character->ActiveStrike.IsValid())
	{
		Character->ActiveStrike->SampleHand(true);
	}
}

void ULightStrikeWindow::NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float DeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(Mesh, Animation, DeltaTime, EventReference);
	if (auto* Character = Cast<ATrainingCharacter>(Mesh->GetOwner()); Character && Character->ActiveStrike.IsValid())
	{
		Character->ActiveStrike->SampleHand(false);
	}
}
