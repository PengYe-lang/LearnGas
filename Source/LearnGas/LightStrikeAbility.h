#pragma once

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LightStrikeAbility.generated.h"

UCLASS()
class LEARNGAS_API ULightStrikeAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	ULightStrikeAbility();
	void SampleHand(bool bFirstSample);
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled) override;

private:
	UPROPERTY() TSoftObjectPtr<UAnimMontage> AttackMontage;
	TSet<TWeakObjectPtr<AActor>> HitActors;
	FVector PreviousHand = FVector::ZeroVector;
	UFUNCTION() void FinishStrike();
	UFUNCTION() void InterruptStrike();
};

UCLASS(DisplayName="Light Strike Hit Window")
class LEARNGAS_API ULightStrikeWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float DeltaTime,
		const FAnimNotifyEventReference& EventReference) override;
};
