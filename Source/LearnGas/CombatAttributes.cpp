#include "CombatAttributes.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UCombatAttributes::UCombatAttributes()
{
	InitMaxHealth(100.f);
	InitHealth(GetMaxHealth());
}

void UCombatAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = FMath::Max(0.f, GetIncomingDamage());
		SetIncomingDamage(0.f);
		SetHealth(FMath::Clamp(GetHealth() - Damage, 0.f, GetMaxHealth()));
	}
}

void UCombatAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UCombatAttributes::OnRep_Health(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributes, Health, Previous);
}

void UCombatAttributes::OnRep_MaxHealth(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributes, MaxHealth, Previous);
}

ULightStrikeDamage::ULightStrikeDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayModifierInfo& Damage = Modifiers.AddDefaulted_GetRef();
	Damage.Attribute = UCombatAttributes::GetIncomingDamageAttribute();
	Damage.ModifierOp = EGameplayModOp::Additive;
	Damage.ModifierMagnitude = FScalableFloat(20.f);
}
