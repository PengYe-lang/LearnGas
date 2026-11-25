#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "TrainingProgressionAttributes.generated.h"

UCLASS()
class UTrainingProgressionAttributes : public UAttributeSet
{
	GENERATED_BODY()

public:
	static constexpr int32 ExperiencePerLevel = 100;

	UTrainingProgressionAttributes();
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Experience, Category="Progression")
	FGameplayAttributeData Experience;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UTrainingProgressionAttributes, Experience)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Experience)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Level, Category="Progression")
	FGameplayAttributeData Level;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UTrainingProgressionAttributes, Level)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Level)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Level)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Level)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SkillPoints, Category="Progression")
	FGameplayAttributeData SkillPoints;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UTrainingProgressionAttributes, SkillPoints)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(SkillPoints)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(SkillPoints)

private:
	UFUNCTION() void OnRep_Experience(const FGameplayAttributeData& Previous);
	UFUNCTION() void OnRep_Level(const FGameplayAttributeData& Previous);
	UFUNCTION() void OnRep_SkillPoints(const FGameplayAttributeData& Previous);
};

UCLASS()
class UTrainingDummyExperience : public UGameplayEffect
{
	GENERATED_BODY()

public:
	static constexpr int32 ExperiencePerDefeat = 50;

	UTrainingDummyExperience();
};
