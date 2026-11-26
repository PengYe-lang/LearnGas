#include "TrainingDefeatReward.h"
#include "TrainingProgressionAttributes.h"

void UTrainingDefeatReward::Grant(const FGameplayEffectContextHandle& DamageContext) const
{
	if (!GetOwner()->HasAuthority()) return;

	UAbilitySystemComponent* Source = DamageContext.GetInstigatorAbilitySystemComponent();
	if (!Source || !Source->HasAttributeSetForAttribute(UTrainingProgressionAttributes::GetExperienceAttribute())) return;

	const auto Reward = Source->MakeOutgoingSpec(UTrainingDummyExperience::StaticClass(), 1.f, Source->MakeEffectContext());
	if (Reward.IsValid()) Source->ApplyGameplayEffectSpecToSelf(*Reward.Data);
}
