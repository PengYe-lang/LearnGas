#include "CombatAttributes.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "Net/UnrealNetwork.h"
#include "TrainingCharacter.h"
#include "TrainingMode.h"
#include "TrainingDefeatReward.h"

UE_DEFINE_GAMEPLAY_TAG(TrainingDown, "State.Training.Down");
UE_DEFINE_GAMEPLAY_TAG(TrainingBurning, "Status.Training.Burning");
UE_DEFINE_GAMEPLAY_TAG(TrainingAttack, "Ability.Training.Attack");
UE_DEFINE_GAMEPLAY_TAG(TrainingWeaponMultiplier, "Data.Training.WeaponMultiplier");
UE_DEFINE_GAMEPLAY_TAG(TrainingDodge, "Ability.Training.Dodge");
UE_DEFINE_GAMEPLAY_TAG(TrainingDodging, "State.Training.Dodging");
UE_DEFINE_GAMEPLAY_TAG(TrainingDodgeInvulnerable, "State.Training.DodgeInvulnerable");
UE_DEFINE_GAMEPLAY_TAG(TrainingHitStun, "State.Training.HitStun");

namespace
{
	void ConfigureInstantModifier(UGameplayEffect& Effect, FGameplayAttribute Attribute, float Magnitude)
	{
		Effect.DurationPolicy = EGameplayEffectDurationType::Instant;
		FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Magnitude);
	}
}

UCombatAttributes::UCombatAttributes()
{
	InitMaxHealth(100.f);
	InitHealth(GetMaxHealth());
	InitStamina(StaminaCapacity);
}

void UCombatAttributes::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = FMath::Max(0.f, GetIncomingDamage());
		SetIncomingDamage(0.f);
		if (Damage <= 0.f || Data.Target.HasMatchingGameplayTag(TrainingDodgeInvulnerable)) return;

		const float PreviousHealth = GetHealth();
		SetHealth(FMath::Clamp(PreviousHealth - Damage, 0.f, GetMaxHealth()));
		const float Applied = PreviousHealth - GetHealth();
		AActor* Victim = Data.Target.GetAvatarActor();
		if (Applied > 0.f && Victim && Victim->HasAuthority())
		{
			const auto* Definition = Cast<UTrainingDamageEffect>(Data.EffectSpec.Def);
			const auto Kind = Data.EffectSpec.GetPeriod() > 0.f ? ETrainingDamageKind::Burning
				: Definition && Definition->bFireDamage ? ETrainingDamageKind::Fire : ETrainingDamageKind::Physical;
			const auto* Source = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();
			auto* Attacker = Source ? Cast<ATrainingPlayerState>(Source->GetOwnerActor()) : nullptr;
			auto* Defender = Cast<ATrainingPlayerState>(Data.Target.GetOwnerActor());
			const FVector Location = Victim->GetActorLocation();
			if (Attacker && Attacker != Defender) Attacker->ClientShowDamage(Location, Applied, Kind, false);
			if (Defender) Defender->ClientShowDamage(Location, Applied, Kind, true);
		}
		if (PreviousHealth > 0.f && GetHealth() <= 0.f)
		{
			AActor* Avatar = Data.Target.GetAvatarActor();
			if (const auto* Reward = Avatar ? Avatar->FindComponentByClass<UTrainingDefeatReward>() : nullptr)
			{
				Reward->Grant(Data.EffectSpec.GetContext());
			}
		}
		if (GetHealth() > 0.f && Data.EffectSpec.GetPeriod() <= 0.f && GetHealth() < PreviousHealth && Cast<ATrainingCharacter>(Data.Target.GetAvatarActor()))
		{
			const auto Stun = Data.Target.MakeOutgoingSpec(UTrainingHitStunEffect::StaticClass(), 1.f, Data.EffectSpec.GetContext());
			if (Stun.IsValid()) Data.Target.ApplyGameplayEffectSpecToSelf(*Stun.Data);
			FGameplayTagContainer Interruptible(TrainingAttack);
			Interruptible.AddTag(TrainingDodge);
			Data.Target.CancelAbilities(&Interruptible);
		}
	}
}

void UCombatAttributes::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCombatAttributes, Stamina, COND_None, REPNOTIFY_Always);
}

void UCombatAttributes::OnRep_FireResistance(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributes, FireResistance, Previous);
}

void UCombatAttributes::OnRep_Armor(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributes, Armor, Previous);
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
	BaseDamage = 20.f;
}

void UCombatAttributes::OnRep_Stamina(const FGameplayAttributeData& Previous)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCombatAttributes, Stamina, Previous);
}

UHeavyStrikeDamage::UHeavyStrikeDamage()
{
	BaseDamage = 45.f;
}

UTrainingPulseDamage::UTrainingPulseDamage()
{
	BaseDamage = 10.f;
}

UTrainingDownEffect::UTrainingDownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("Down"));
	FInheritedTagContainer Changes;
	Changes.AddTag(TrainingDown);
	Tags->SetAndApplyTargetTagChanges(Changes);
	GEComponents.Add(Tags);
}

UTrainingHitStunEffect::UTrainingHitStunEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.40f);
	auto* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("HitStun"));
	FInheritedTagContainer Changes;
	Changes.AddTag(TrainingHitStun);
	Tags->SetAndApplyTargetTagChanges(Changes);
	GEComponents.Add(Tags);
}

ULightStrikeCost::ULightStrikeCost()
{
	ConfigureInstantModifier(*this, UCombatAttributes::GetStaminaAttribute(), -12.f);
}

UHeavyStrikeCost::UHeavyStrikeCost()
{
	ConfigureInstantModifier(*this, UCombatAttributes::GetStaminaAttribute(), -30.f);
}

UTrainingDodgeCost::UTrainingDodgeCost()
{
	ConfigureInstantModifier(*this, UCombatAttributes::GetStaminaAttribute(), -24.f);
}

UTrainingDodgeInvulnerabilityEffect::UTrainingDodgeInvulnerabilityEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(0.24f);
	auto* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("DodgeInvulnerable"));
	FInheritedTagContainer Changes;
	Changes.AddTag(TrainingDodgeInvulnerable);
	Tags->SetAndApplyTargetTagChanges(Changes);
	GEComponents.Add(Tags);
}
