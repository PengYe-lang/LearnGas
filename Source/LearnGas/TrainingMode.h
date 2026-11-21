#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "TrainingMode.generated.h"

class UCombatAttributes;

UCLASS()
class LEARNGAS_API ATrainingPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ATrainingPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UCombatAttributes> Attributes;
};

UCLASS()
class LEARNGAS_API ATrainingMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ATrainingMode();
};
