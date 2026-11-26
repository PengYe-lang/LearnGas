#include "TrainingDamage.h"
#include "CombatAttributes.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
	const FGameplayEffectAttributeCaptureDefinition& DefenseCapture(bool bFire)
	{
		static const FGameplayEffectAttributeCaptureDefinition Armor(UCombatAttributes::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
		static const FGameplayEffectAttributeCaptureDefinition Fire(UCombatAttributes::GetFireResistanceAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
		return bFire ? Fire : Armor;
	}
}

UTrainingDamageEffect::UTrainingDamageEffect()
{
	Executions.AddDefaulted_GetRef().CalculationClass = UTrainingDamageExecution::StaticClass();
}

UTrainingDamageExecution::UTrainingDamageExecution()
{
	RelevantAttributesToCapture.Add(DefenseCapture(false));
	RelevantAttributesToCapture.Add(DefenseCapture(true));
}

void UTrainingDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Params,
	FGameplayEffectCustomExecutionOutput& Output) const
{
	const auto& Spec = Params.GetOwningSpec();
	const auto* Effect = CastChecked<UTrainingDamageEffect>(Spec.Def);
	FAggregatorEvaluateParameters Eval;
	Eval.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	Eval.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	float Defense = 0.f;
	Params.AttemptCalculateCapturedAttributeMagnitude(DefenseCapture(Effect->bFireDamage), Eval, Defense);
	const float Factor = Effect->bFireDamage ? 1.f - FMath::Clamp(Defense, 0.f, 100.f) / 100.f
		: 100.f / (100.f + FMath::Max(0.f, Defense));
	const float Damage = Effect->BaseDamage * Spec.GetSetByCallerMagnitude(TrainingWeaponMultiplier, false, 1.f) * Factor;
	if (Damage > 0.f) Output.AddOutputModifier(FGameplayModifierEvaluatedData(UCombatAttributes::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage));
}

UTrainingBurn::UTrainingBurn()
{
	BaseDamage = 5.f;
	bFireDamage = true;
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(3.25f);
	Period = FScalableFloat(1.f);
	bExecutePeriodicEffectOnApplication = false;
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 1;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::NeverReset;
	auto* Tags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("Burning"));
	FInheritedTagContainer Changes;
	Changes.AddTag(TrainingBurning);
	Tags->SetAndApplyTargetTagChanges(Changes);
	GEComponents.Add(Tags);
}
