#pragma once

#include "Abilities/GameplayAbility.h"
#include "TrainingDodgeAbility.generated.h"

UCLASS()
class UTrainingDodgeAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UTrainingDodgeAbility();
	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* RelevantTags) const override;
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;

private:
	UFUNCTION() void FinishDodge();
};
