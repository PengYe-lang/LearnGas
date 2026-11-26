#pragma once

#include "GameFramework/HUD.h"
#include "TrainingDamage.h"
#include "TrainingHUD.generated.h"

UCLASS()
class LEARNGAS_API ATrainingHUD : public AHUD
{
	GENERATED_BODY()
public:
	ATrainingHUD();
	virtual void DrawHUD() override;
	void ShowDamage(FVector Location, float Amount, ETrainingDamageKind Kind, bool bTaken);
	void ToggleSkillBook() { bSkillBookOpen = !bSkillBookOpen; }

private:
	struct FDamageNumber
	{
		FVector Origin;
		float Amount;
		ETrainingDamageKind Kind;
		bool bTaken;
		double Start;
		float Shift;
	};
	TArray<FDamageNumber> DamageNumbers;
	int32 NextDamageLane = 0;
	bool bSkillBookOpen = false;
	UPROPERTY() TObjectPtr<UFont> LabelFont;
};
