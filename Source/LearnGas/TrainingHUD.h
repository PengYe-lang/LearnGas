#pragma once

#include "GameFramework/HUD.h"
#include "TrainingHUD.generated.h"

UCLASS()
class LEARNGAS_API ATrainingHUD : public AHUD
{
	GENERATED_BODY()
public:
	ATrainingHUD();
	virtual void DrawHUD() override;
	void ToggleSkillBook() { bSkillBookOpen = !bSkillBookOpen; }

private:
	bool bSkillBookOpen = false;
	UPROPERTY() TObjectPtr<UFont> LabelFont;
};
