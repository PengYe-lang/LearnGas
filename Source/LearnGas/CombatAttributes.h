#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "CombatAttributes.generated.h"

UCLASS()
class LEARNGAS_API UCombatAttributes : public UAttributeSet
{
	GENERATED_BODY()
public:
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

	UPROPERTY()
	FGameplayAttributeData IncomingDamage;
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCombatAttributes, IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(IncomingDamage)
	GAMEPLAYATTRIBUTE_VALUE_SETTER(IncomingDamage)

private:
	UFUNCTION() void OnRep_Health(const FGameplayAttributeData& Previous);
	UFUNCTION() void OnRep_MaxHealth(const FGameplayAttributeData& Previous);
};

UCLASS()
class LEARNGAS_API ULightStrikeDamage : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULightStrikeDamage();
};
