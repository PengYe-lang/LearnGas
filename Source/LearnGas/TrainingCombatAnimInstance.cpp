#include "TrainingCombatAnimInstance.h"
#include "Animation/AnimMontage.h"

void UTrainingCombatAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	KickLowerBodyWeight = 0.f;
	const UAnimMontage* Montage = GetCurrentActiveMontage();
	static const FName KickSection(TEXT("Light3"));
	if (!Montage || Montage_GetCurrentSection(Montage) != KickSection)
	{
		return;
	}

	float StartTime;
	float EndTime;
	Montage->GetSectionStartAndEndTime(Montage->GetSectionIndex(KickSection), StartTime, EndTime);
	const float Time = Montage_GetPosition(Montage) - StartTime;
	// Give the kick control of the legs during the chamber, strike, and retraction.
	const float Enter = FMath::SmoothStep(0.10f, 0.25f, Time);
	const float Leave = 1.f - FMath::SmoothStep(0.63f, 0.86f, Time);
	KickLowerBodyWeight = Enter * Leave;
}
