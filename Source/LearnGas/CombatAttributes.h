#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "NativeGameplayTags.h"
#include "CombatAttributes.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingAttack);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingDodging);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TrainingHitStun);

UCLASS()
class LEARNGAS_API UCombatAttributes : public UAttributeSet
{
	GENERATED_BODY()
public:
	static constexpr float StaminaCapacity = 100.f;
	UCombatAttributes();
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Health, Category="Combat")
	FGameplayAttributeData Health;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCombatAttributes, Health)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Health)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Health)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="Combat")
	FGameplayAttributeData MaxHealth;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCombatAttributes, MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_Stamina, Category="Combat")
	FGameplayAttributeData Stamina;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCombatAttributes, Stamina)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Stamina)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Stamina)
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Stamina)

	UPROPERTY()
	FGameplayAttributeData IncomingDamage;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCombatAttributes, IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(IncomingDamage)

private:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Previous);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Previous);
	UFUNCTION() void OnRep_Stamina(const FGameplayAttributeData& Previous);
};

UCLASS()
class UHeavyStrikeDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UHeavyStrikeDamage();
};

UCLASS()
class UTrainingPulseDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingPulseDamage();
};

UCLASS()
class UTrainingHitStunEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingHitStunEffect();
};

UCLASS()
class ULightStrikeCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULightStrikeCost();
};

UCLASS()
class UHeavyStrikeCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UHeavyStrikeCost();
};

UCLASS()
class UTrainingDodgeCost : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UTrainingDodgeCost();
};

UCLASS()
class LEARNGAS_API ULightStrikeDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULightStrikeDamage();
};
