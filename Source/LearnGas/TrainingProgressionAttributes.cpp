#include "TrainingProgressionAttributes.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UTrainingProgressionAttributes::UTrainingProgressionAttributes()
{
	InitLevel(1.f);
}

void UTrainingProgressionAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute != GetExperienceAttribute()) return;

	const float LevelFromExperience = 1.f + FMath::FloorToFloat(GetExperience() / ExperiencePerLevel);
	const float LevelsGained = LevelFromExperience - GetLevel();
	if (LevelsGained > 0.f)
	{
		SetLevel(LevelFromExperience);
		SetSkillPoints(GetSkillPoints() + LevelsGained);
	}
}

void UTrainingProgressionAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UTrainingProgressionAttributes, Experience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTrainingProgressionAttributes, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UTrainingProgressionAttributes, SkillPoints, COND_OwnerOnly, REPNOTIFY_Always);
}

void UTrainingProgressionAttributes::OnRep_Experience(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTrainingProgressionAttributes, Experience, Previous);
}

void UTrainingProgressionAttributes::OnRep_Level(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTrainingProgressionAttributes, Level, Previous);
}

void UTrainingProgressionAttributes::OnRep_SkillPoints(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UTrainingProgressionAttributes, SkillPoints, Previous);
}

UTrainingDummyExperience::UTrainingDummyExperience()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Reward = Modifiers.AddDefaulted_GetRef();
	Reward.Attribute = UTrainingProgressionAttributes::GetExperienceAttribute();
	Reward.ModifierOp = EGameplayModOp::Additive;
	Reward.ModifierMagnitude = FScalableFloat(ExperiencePerDefeat);
}

UTrainingTrialExperience::UTrainingTrialExperience()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Reward = Modifiers.AddDefaulted_GetRef();
	Reward.Attribute = UTrainingProgressionAttributes::GetExperienceAttribute();
	Reward.ModifierOp = EGameplayModOp::Additive;
	Reward.ModifierMagnitude = FScalableFloat(100.f);
}
