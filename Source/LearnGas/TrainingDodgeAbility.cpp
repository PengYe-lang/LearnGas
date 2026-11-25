#include "TrainingDodgeAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "CombatAttributes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LightStrikeAbility.h"
#include "TrainingCharacter.h"

UTrainingDodgeAbility::UTrainingDodgeAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	DodgeMontage = FSoftObjectPath(TEXT("/Game/Combat/AM_Dodge.AM_Dodge"));
	CostGameplayEffectClass = UTrainingDodgeCost::StaticClass();
	SetAssetTags(FGameplayTagContainer(TrainingDodge));
	ActivationOwnedTags.AddTag(TrainingDodging);
	ActivationBlockedTags.AddTag(TrainingHitStun);
}

bool UTrainingDodgeAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* RelevantTags) const
{
	const auto* Character = Info ? Cast<ATrainingCharacter>(Info->AvatarActor.Get()) : nullptr;
	if (!Character || !Character->GetCharacterMovement()->IsMovingOnGround()) return false;

	const auto* Strike = Character->ActiveStrike.Get();
	return (!Strike || Strike->IsRecovering())
		&& Super::CanActivateAbility(Handle, Info, SourceTags, TargetTags, RelevantTags);
}

void UTrainingDodgeAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	Super::ActivateAbility(Handle, Info, Activation, Event);
	auto* Character = Cast<ATrainingCharacter>(Info->AvatarActor.Get());
	auto* Montage = DodgeMontage.LoadSynchronous();
	if (!Character || !Info->GetAnimInstance() || !Montage || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, true, true);
		return;
	}

	const FGameplayTagContainer Attacks(TrainingAttack);
	GetAbilitySystemComponentFromActorInfo()->CancelAbilities(&Attacks);
	FVector Direction = Character->GetPendingMovementInputVector();
	if (Direction.IsNearlyZero()) Direction = Character->GetLastMovementInputVector();
	if (Direction.IsNearlyZero()) Direction = Character->GetCharacterMovement()->GetCurrentAcceleration();
	if (!Direction.IsNearlyZero()) Character->SetActorRotation(Direction.GetSafeNormal2D().Rotation());

	auto* Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	Play->OnCompleted.AddDynamic(this, &ThisClass::FinishDodge);
	Play->OnInterrupted.AddDynamic(this, &ThisClass::InterruptDodge);
	Play->OnCancelled.AddDynamic(this, &ThisClass::InterruptDodge);
	Play->ReadyForActivation();
	if (IsActive() && HasAuthority(&Activation))
	{
		Invulnerability = ApplyGameplayEffectToOwner(Handle, Info, Activation,
			GetDefault<UTrainingDodgeInvulnerabilityEffect>(), 1.f);
	}
}

void UTrainingDodgeAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (auto* ASC = GetAbilitySystemComponentFromActorInfo(); ASC && Invulnerability.IsValid())
	{
		const auto Effect = Invulnerability;
		Invulnerability.Invalidate();
		ASC->RemoveActiveGameplayEffect(Effect);
	}
	Super::EndAbility(Handle, Info, Activation, bReplicateEndAbility, bWasCancelled);
}

void UTrainingDodgeAbility::FinishDodge()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UTrainingDodgeAbility::InterruptDodge()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
