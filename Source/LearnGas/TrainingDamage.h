#pragma once

#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "TrainingDamage.generated.h"

UCLASS(Abstract)
class UTrainingDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingDamageEffect();
	UPROPERTY(EditDefaultsOnly, Category="Damage") float BaseDamage = 0.f;
	UPROPERTY(EditDefaultsOnly, Category="Damage") bool bFireDamage = false;
};

UCLASS()
class UTrainingDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UTrainingDamageExecution();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Params,
		FGameplayEffectCustomExecutionOutput& Output) const override;
};

UCLASS()
class UTrainingBurn : public UTrainingDamageEffect
{
	GENERATED_BODY()
public:
	UTrainingBurn();
};
