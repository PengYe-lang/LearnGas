#pragma once

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TrainingDamage.h"
#include "TrainingFirebolt.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class ATrainingCrosshairTarget : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
public:
	virtual void ConfirmTargetingAndContinue() override;
};

UCLASS()
class UTrainingFireboltHit : public UTrainingDamageEffect
{
	GENERATED_BODY()
public:
	UTrainingFireboltHit();
};

UCLASS()
class UTrainingFireboltCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingFireboltCost();
};

UCLASS()
class ATrainingFirebolt : public AActor
{
	GENERATED_BODY()
public:
	ATrainingFirebolt();
	FGameplayEffectSpecHandle Damage;
	virtual void PostInitializeComponents() override;
private:
	UPROPERTY() TObjectPtr<USphereComponent> Sphere;
	UPROPERTY() TObjectPtr<UProjectileMovementComponent> Movement;
	UFUNCTION() void Impact(const FHitResult& Hit);
};

UCLASS()
class UTrainingFireboltAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UTrainingFireboltAbility();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;
private:
	UFUNCTION() void Fire(const FGameplayAbilityTargetDataHandle& Data);
	UFUNCTION() void CancelAim(const FGameplayAbilityTargetDataHandle& Data);
	UFUNCTION() void AimExpired();
};
