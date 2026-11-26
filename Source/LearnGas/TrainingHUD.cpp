#include "TrainingHUD.h"
#include "TrainingDummy.h"
#include "TrainingPulse.h"
#include "TrainingCharacter.h"
#include "TrainingMode.h"
#include "TrainingProgressionAttributes.h"
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
	Text(Margin, Canvas->SizeY - 42 * Scale, GetNetMode() == NM_Standalone
		? TEXT("WASD 移动    左键 轻击    左 Shift 闪步    F6 保存    F9 读取")
		: TEXT("WASD 移动    左键 轻击    左 Shift 闪步"), 15);
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
			if (const auto* State = Character->GetPlayerState<ATrainingPlayerState>())
			{
				const int32 Level = FMath::RoundToInt(ASC->GetNumericAttribute(UTrainingProgressionAttributes::GetLevelAttribute()));
				const int32 Experience = FMath::RoundToInt(ASC->GetNumericAttribute(UTrainingProgressionAttributes::GetExperienceAttribute()));
				const int32 Points = FMath::RoundToInt(ASC->GetNumericAttribute(UTrainingProgressionAttributes::GetSkillPointsAttribute()));
				const int32 Goal = UTrainingProgressionAttributes::ExperiencePerLevel;
				const int32 CurrentExperience = FMath::Clamp(Experience - (Level - 1) * Goal, 0, Goal);
				Text(Margin, Margin + 55 * Scale, FString::Printf(TEXT("等级 %d    经验 %d / %d    技能点 %d"),
					Level, CurrentExperience, Goal, Points), 15);
				FString HeavyStrikeStatus;
				if (State->HasLearnedHeavyStrike())
				{
					HeavyStrikeStatus = TEXT("重击已学会    轻二收招时按右键");
				}
				else if (State->CanLearnHeavyStrike())
				{
					HeavyStrikeStatus = FString::Printf(TEXT("按 K 学习重击    消耗 %d 技能点"), State->HeavyStrikeSkillPointCost);
				}
				else
				{
					HeavyStrikeStatus = FString::Printf(TEXT("重击需要等级 %d 和 %d 技能点    击倒训练靶 +%d 经验"),
						State->HeavyStrikeRequiredLevel, State->HeavyStrikeSkillPointCost, UTrainingDummyExperience::ExperiencePerDefeat);
				}
				Text(Margin, Margin + 82 * Scale, HeavyStrikeStatus, 15);
				Text(Margin, Margin + 108 * Scale, FString::Printf(TEXT("%s   ×%g    Q 切换"),
					State->HasGauntlets() ? TEXT("训练拳套") : TEXT("徒手"), State->GetWeaponMultiplier()), 15);
				const FString Notice = State->GetCheckpointNotice();
				if (!Notice.IsEmpty()) Text(Margin, Canvas->SizeY - 78 * Scale, Notice, 15);
			}
			if (ASC->HasMatchingGameplayTag(TrainingHitStun))
			{
				Text(Margin, Margin + 138 * Scale, TEXT("受击硬直"), 15);
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
