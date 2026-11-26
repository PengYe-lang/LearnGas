#pragma once

#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "TrainingArmorZone.generated.h"

class UBoxComponent;

UCLASS()
class UTrainingArmorBuff : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingArmorBuff();
};

UCLASS()
class ATrainingArmorZone : public AActor
{
	GENERATED_BODY()
public:
	ATrainingArmorZone();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY() TObjectPtr<UBoxComponent> Area;
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle> Grants;
	FTimerHandle RefreshTimer;
	void RefreshOccupants();
};
