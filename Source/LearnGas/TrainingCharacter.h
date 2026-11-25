#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "TrainingCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class ULightStrikeAbility;
struct FInputActionValue;

UCLASS()
class LEARNGAS_API ATrainingCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ATrainingCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void UnPossessed() override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	virtual void PawnClientRestart() override;

	TWeakObjectPtr<ULightStrikeAbility> ActiveStrike;

private:
	void InitializeAbilitySystem();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Strike();
	void HeavyStrike();
	void LearnHeavyStrike();
	void Dodge();

	UPROPERTY() TObjectPtr<UInputMappingContext> MovementContext;
	UPROPERTY() TObjectPtr<UInputMappingContext> MouseContext;
	UPROPERTY() TSoftObjectPtr<UInputMappingContext> CombatContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> MouseAction;
	UPROPERTY() TSoftObjectPtr<UInputAction> StrikeAction;
	UPROPERTY() TSoftObjectPtr<UInputAction> HeavyStrikeAction;
	UPROPERTY() TSoftObjectPtr<UInputAction> DodgeAction;
};
