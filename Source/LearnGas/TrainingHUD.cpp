#include "TrainingHUD.h"
#include "TrainingDummy.h"
#include "CombatAttributes.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "Engine/Font.h"

ATrainingHUD::ATrainingHUD()
{
	static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/Roboto"));
	LabelFont = Font.Object;
}

void ATrainingHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	const float Scale = FMath::Clamp(Canvas->SizeY / 800.f, .75f, 1.5f);
	auto Text = [&](float X, float Y, const FString& Label, int32 Size, bool bCenter = false)
	{
		FCanvasTextItem Item(FVector2D(X,Y), FText::FromString(Label),
			FSlateFontInfo(LabelFont, FMath::RoundToInt(Size * Scale), TEXT("Bold")), FLinearColor::White);
		Item.bCentreX = bCenter;
		Item.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Item);
	};

	Text(24 * Scale, Canvas->SizeY - 42 * Scale, TEXT("WASD 移动    鼠标 转动镜头    左键 轻击"), 15);
	for (TActorIterator<ATrainingDummy> It(GetWorld()); It; ++It)
	{
		FVector2D Screen;
		if (!PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0,0,100), Screen, true)) continue;
		const UCombatAttributes* Attr = It->GetAttributes();
		const float Width = 154 * Scale;
		const float X = Screen.X - Width * .5f;
		Text(Screen.X, Screen.Y - 28 * Scale, FString::Printf(TEXT("训练靶   %.0f / %.0f"), Attr->GetHealth(), Attr->GetMaxHealth()), 14, true);
		DrawRect(FLinearColor(.02f,.035f,.05f,.9f), X, Screen.Y, Width, 7 * Scale);
		DrawRect(FLinearColor(.12f,.88f,.64f), X, Screen.Y, Width * Attr->GetHealth() / Attr->GetMaxHealth(), 7 * Scale);
	}
}
