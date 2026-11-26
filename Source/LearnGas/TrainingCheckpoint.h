#pragma once

#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "TrainingCheckpoint.generated.h"

UCLASS()
class UTrainingCheckpoint : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY() int32 Version = 1;
	UPROPERTY() int64 Generation = 0;
	UPROPERTY() int32 Experience = 0;
	UPROPERTY() int32 SkillPoints = 0;
	UPROPERTY() FGameplayTagContainer LearnedSkills;
	UPROPERTY() FGameplayTag PassiveSkill;
	bool HasSkill(FGameplayTag Id) const;
	UPROPERTY() bool bTrialCleared = false;
	UPROPERTY() bool bLearnedHeavy = false;
	UPROPERTY() bool bGauntlets = false;
	UPROPERTY() float Health = 0.f;
	UPROPERTY() float Stamina = 0.f;

	static UTrainingCheckpoint* LoadLatest(int32& SlotIndex);
	bool WriteNext();
	bool IsValidProgress() const;
};
