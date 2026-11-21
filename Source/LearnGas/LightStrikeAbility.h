#pragma once

#include "Abilities/GameplayAbility.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LightStrikeAbility.generated.h"

UCLASS()
class LEARNGAS_API ULightStrikeAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	ULightStrikeAbility();
	void BufferNextStrike();
	void SetBufferWindow(const UAnimSequenceBase* Animation, bool bOpen);
	void ConsumeBufferedStrike(const UAnimSequenceBase* Animation);
	void SampleStrike(const UAnimSequenceBase* Animation, FName HitSocket, bool bFirstSample);
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled) override;

private:
	UPROPERTY() TSoftObjectPtr<UAnimMontage> AttackMontage;
	TSet<TWeakObjectPtr<AActor>> HitActors;
	FVector PreviousStrikePosition = FVector::ZeroVector;
	FName HitSection;
	FName OpenBufferSection;
	FName BufferedSection;

	FName GetPlayingSection(const UAnimSequenceBase* Animation) const;
	void ResetCombo();
	UFUNCTION() void FinishStrike();
	UFUNCTION() void InterruptStrike();
};

UCLASS(DisplayName="Light Strike Hit Window")
class LEARNGAS_API ULightStrikeWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Strike")
	FName HitSocket = TEXT("hand_r");

	virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float DeltaTime,
		const FAnimNotifyEventReference& EventReference) override;
};

UCLASS(DisplayName="Light Strike Input Buffer")
class LEARNGAS_API ULightStrikeBufferWindow : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	ULightStrikeBufferWindow();
	virtual void NotifyBegin(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation, float Duration,
		const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};

UCLASS(DisplayName="Light Strike Combo Decision")
class LEARNGAS_API ULightStrikeComboDecision : public UAnimNotify
{
	GENERATED_BODY()
public:
	ULightStrikeComboDecision();
	virtual void Notify(USkeletalMeshComponent* Mesh, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
