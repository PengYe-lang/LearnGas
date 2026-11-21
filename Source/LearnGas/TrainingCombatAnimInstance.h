#pragma once

#include "Animation/AnimInstance.h"
#include "TrainingCombatAnimInstance.generated.h"

UCLASS()
class LEARNGAS_API UTrainingCombatAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="Combat")
	float KickLowerBodyWeight = 0.f;
};
