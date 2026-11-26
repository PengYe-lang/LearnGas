#pragma once

#include "Components/ActorComponent.h"
#include "TrainingDefeatReward.generated.h"

struct FGameplayEffectContextHandle;

UCLASS()
class UTrainingDefeatReward : public UActorComponent
{
	GENERATED_BODY()
public:
	void Grant(const FGameplayEffectContextHandle& DamageContext) const;
};
