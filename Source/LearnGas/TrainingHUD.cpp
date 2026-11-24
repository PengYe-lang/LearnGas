#include "TrainingHUD.h"
#include "TrainingDummy.h"
#include "TrainingPulse.h"
#include "TrainingCharacter.h"
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

	const float Margin = 24.f * Scale;
	Text(Margin, Canvas->SizeY - 42 * Scale, TEXT("WASD 移动    左键 轻击    轻二后右键 重击    左 Shift 闪步"), 15);
	if (const auto* Character = Cast<ATrainingCharacter>(PlayerOwner->GetPawn()))
	{
		if (const auto* ASC = Character->GetAbilitySystemComponent())
		{
			const float HP = ASC->GetNumericAttribute(UCombatAttributes::GetHealthAttribute());
			const float Stamina = ASC->GetNumericAttribute(UCombatAttributes::GetStaminaAttribute());
			Text(Margin, Margin, FString::Printf(TEXT("生命 %.0f    体力 %.0f / 100"), HP, Stamina), 18);
			DrawRect(FLinearColor(.03f, .04f, .05f), Margin, Margin + 36 * Scale, 220 * Scale, 7 * Scale);
			DrawRect(FLinearColor(.95f, .73f, .25f), Margin, Margin + 36 * Scale,
				220 * Scale * Stamina / UCombatAttributes::StaminaCapacity, 7 * Scale);
			if (ASC->HasMatchingGameplayTag(TrainingHitStun))
			{
				Text(Margin, Margin + 50 * Scale, TEXT("受击硬直"), 15);
			}
		}
	}
	for (TActorIterator<ATrainingPulse> It(GetWorld()); It; ++It)
	{
		FVector2D Screen;
		if (PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0, 0, 120), Screen, true))
		{
			Text(Screen.X, Screen.Y, It->IsWindingUp() ? TEXT("脉冲即将释放") : TEXT("受击打断测试区"), 15, true);
		}
	}
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
