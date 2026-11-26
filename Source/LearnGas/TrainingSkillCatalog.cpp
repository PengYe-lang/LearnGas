#include "TrainingSkillCatalog.h"
#include "TrainingFirebolt.h"
#include "CombatAttributes.h"
#include "LightStrikeAbility.h"

UE_DEFINE_GAMEPLAY_TAG(TrainingHeavySkill, "Skill.Training.Heavy");
UE_DEFINE_GAMEPLAY_TAG(TrainingBoltSkill, "Skill.Training.Firebolt");
UE_DEFINE_GAMEPLAY_TAG(TrainingWardSkill, "Skill.Training.Ward");
UE_DEFINE_GAMEPLAY_TAG(TrainingPassiveSlot, "Slot.Training.Passive");

TConstArrayView<FTrainingSkillDefinition> TrainingSkills::All()
{
	static const FTrainingSkillDefinition Skills[] = {
		{ TrainingHeavySkill, UHeavyStrikeAbility::StaticClass(), TEXT("重击"), TEXT("轻二后按右键，打出重击"), 2, 1 },
		{ TrainingBoltSkill, UTrainingFireboltAbility::StaticClass(), TEXT("焰弹"), TEXT("R 向准星发射，消耗 18 体力，命中造成 24 火伤"), 2, 1 },
		{ TrainingWardSkill, UTrainingWardAbility::StaticClass(), TEXT("守势"), TEXT("装入被动槽后获得 25 护甲"), 2, 1 }
	};
	return MakeArrayView(Skills);
}

const FTrainingSkillDefinition* TrainingSkills::Find(FGameplayTag Id)
{
	for (const auto& Skill : All())
		if (Skill.Id == Id) return &Skill;
	return nullptr;
}

UTrainingWardEffect::UTrainingWardEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto& Mod = Modifiers.AddDefaulted_GetRef();
	Mod.Attribute = UCombatAttributes::GetArmorAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FScalableFloat(25.f);
}

UTrainingWardAbility::UTrainingWardAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UTrainingWardAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event)
{
	if (CommitAbility(Handle, Info, Activation))
		Ward = ApplyGameplayEffectToOwner(Handle, Info, Activation, GetDefault<UTrainingWardEffect>(), 1.f);
	if (!Ward.IsValid()) EndAbility(Handle, Info, Activation, false, true);
}

void UTrainingWardAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
	FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled)
{
	if (Ward.IsValid() && Info->AbilitySystemComponent.IsValid()) Info->AbilitySystemComponent->RemoveActiveGameplayEffect(Ward);
	Ward.Invalidate();
	Super::EndAbility(Handle, Info, Activation, bReplicate, bCancelled);
}
