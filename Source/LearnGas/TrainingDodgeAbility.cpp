#include "TrainingDodgeAbility.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "CombatAttributes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LightStrikeAbility.h"
#include "TrainingCharacter.h"

UTrainingDodgeAbility::UTrainingDodgeAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	CostGameplayEffectClass = UTrainingDodgeCost::StaticClass();
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
	if (!Character || !CommitAbility(Handle, Info, Activation))
	{
		EndAbility(Handle, Info, Activation, true, true);
		return;
	}

	const FGameplayTagContainer Attacks(TrainingAttack);
	GetAbilitySystemComponentFromActorInfo()->CancelAbilities(&Attacks);
	const FVector Input = Character->GetLastMovementInputVector().GetSafeNormal2D();
	const FVector Direction = Input.IsNearlyZero() ? Character->GetActorForwardVector() : Input;
	auto* Dash = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, TEXT("Dodge"), Direction, 950.f, .24f, false, nullptr, ERootMotionFinishVelocityMode::ClampVelocity,
		FVector::ZeroVector, Character->GetCharacterMovement()->MaxWalkSpeed, true);
	Dash->OnFinish.AddDynamic(this, &ThisClass::FinishDodge);
	Dash->ReadyForActivation();
}

void UTrainingDodgeAbility::FinishDodge()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
