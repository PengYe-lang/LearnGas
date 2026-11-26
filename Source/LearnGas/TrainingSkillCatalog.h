#pragma once

#include "Abilities/GameplayAbility.h"
#include "NativeGameplayTags.h"
#include "GameplayEffect.h"
#include "TrainingSkillCatalog.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingHeavySkill);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingWardSkill);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingPassiveSlot);

struct FTrainingSkillDefinition
{
	FGameplayTag Id;
	TSubclassOf<UGameplayAbility> Ability;
	const TCHAR* Name;
	const TCHAR* Description;
	int32 RequiredLevel;
	int32 Cost;
};

namespace TrainingSkills
{
	TConstArrayView<FTrainingSkillDefinition> All();
	const FTrainingSkillDefinition* Find(FGameplayTag Id);
}

UCLASS()
class UTrainingWardEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingWardEffect();
};

UCLASS()
class UTrainingWardAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UTrainingWardAbility();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled) override;
private:
	FActiveGameplayEffectHandle Ward;
};
