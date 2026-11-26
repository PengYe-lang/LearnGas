#pragma once

#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "TrainingDummy.generated.h"

class UCombatAttributes;
struct FGameplayEffectContextHandle;
struct FOnAttributeChangeData;

UCLASS()
class LEARNGAS_API ATrainingDummy : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ATrainingDummy();
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	const UCombatAttributes* GetAttributes() const { return Attributes; }

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UCombatAttributes> Attributes;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Rim;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ring;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Bullseye;

	void HealthChanged(const FOnAttributeChangeData& Change);
	void RestoreHealth();
	FTimerHandle ResetTimer;
};
