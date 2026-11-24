#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "TrainingMode.generated.h"

class UCombatAttributes;
struct FOnAttributeChangeData;

UCLASS()
class LEARNGAS_API ATrainingPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ATrainingPlayerState();
	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UCombatAttributes> Attributes;
	FTimerHandle StaminaRegen;

	void OnStaminaChanged(const FOnAttributeChangeData& Change);
	void RegenerateStamina();
};

UCLASS()
class LEARNGAS_API ATrainingMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ATrainingMode();
};
