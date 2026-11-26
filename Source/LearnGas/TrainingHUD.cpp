#include "TrainingHUD.h"
#include "TrainingSentinel.h"
#include "TrainingSkillCatalog.h"
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
			Text(Margin, Margin, FString::Printf(TEXT("生命 %.0f    体力 %.0f / 100    护甲 %.0f    火抗 %.0f%%"), HP, Stamina,
				ASC->GetNumericAttribute(UCombatAttributes::GetArmorAttribute()), ASC->GetNumericAttribute(UCombatAttributes::GetFireResistanceAttribute())), 18);
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
				const FString HeavyStatus = State->HasSkill(TrainingHeavySkill) ? TEXT("重击已学会 · 轻二后按右键")
					: State->CanLearn(TrainingHeavySkill) ? TEXT("K 学习重击") : TEXT("击倒训练靶 +50 经验");
				Text(Margin, Margin + 82 * Scale, HeavyStatus + TEXT("    Tab 技能册"), 15);
				if (State->HasSkill(TrainingBoltSkill))
				{
					const FVector2D Center(Canvas->SizeX * .5f, Canvas->SizeY * .5f);
					DrawRect(FLinearColor::Black, Center.X - 3, Center.Y - 3, 6, 6);
					DrawRect(FLinearColor::White, Center.X - 1.5f, Center.Y - 1.5f, 3, 3);
				}
				if (bSkillBookOpen)
				{
					const float Top = Margin + 170 * Scale;
					DrawRect(FLinearColor(.025f,.04f,.06f,.93f), Margin, Top, 445 * Scale, (46 + 40 * TrainingSkills::All().Num()) * Scale);
					Text(Margin + 12 * Scale, Top + 6 * Scale, TEXT("技能册    K 重击   B 焰弹   J 守势   P 装卸守势"), 15);
					int32 Row = 0;
					for (const auto& Skill : TrainingSkills::All())
					{
						const bool bWard = Skill.Id == TrainingWardSkill;
						const TCHAR* Status = !State->HasSkill(Skill.Id) ? (State->CanLearn(Skill.Id) ? TEXT("可学习") : TEXT("未解锁"))
							: bWard ? (State->IsWardEquipped() ? TEXT("已装备") : TEXT("未装备")) : TEXT("已学习");
						Text(Margin + 12 * Scale, Top + (31 + Row * 40) * Scale, FString::Printf(TEXT("%s · %s    Lv.%d / %d 点"),
							Skill.Name, Status, Skill.RequiredLevel, Skill.Cost), 14);
						Text(Margin + 22 * Scale, Top + (49 + Row++ * 40) * Scale, Skill.Description, 13);
					}
				}
				Text(Margin, Margin + 108 * Scale, FString::Printf(TEXT("%s   ×%g    Q 切换"),
					State->HasGauntlets() ? TEXT("训练拳套 · 轻三点燃") : TEXT("徒手"), State->GetWeaponMultiplier()), 15);
				const FString Notice = State->GetCheckpointNotice();
				if (!Notice.IsEmpty()) Text(Margin, Canvas->SizeY - 78 * Scale, Notice, 15);
			}
			if (ASC->HasMatchingGameplayTag(TrainingDown))
			{
				Text(Canvas->SizeX * .5f, Canvas->SizeY * .42f, TEXT("已倒下 · 即将返回训练起点"), 23, true);
			}
			else if (ASC->HasMatchingGameplayTag(TrainingHitStun))
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
	for (TActorIterator<ATrainingSentinel> It(GetWorld()); It; ++It)
	{
		FVector2D Screen;
		if (!PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0,0,112), Screen, true)) continue;
		const auto Phase = It->GetPhase();
		const TCHAR* Label = Phase == ETrainingSentinelPhase::Down ? TEXT("守卫已倒下")
			: Phase == ETrainingSentinelPhase::Windup ? TEXT("守卫蓄力 · 准备闪避")
			: Phase == ETrainingSentinelPhase::Swing ? TEXT("守卫出拳") : TEXT("陪练守卫");
		const float HP = It->GetAbilitySystemComponent()->GetNumericAttribute(UCombatAttributes::GetHealthAttribute());
		Text(Screen.X, Screen.Y, FString::Printf(TEXT("%s   %.0f"), Label, HP), 15, true);
	}
	for (TActorIterator<ATrainingDummy> It(GetWorld()); It; ++It)
	{
		FVector2D Screen;
		if (!PlayerOwner->ProjectWorldLocationToScreen(It->GetActorLocation() + FVector(0,0,100), Screen, true)) continue;
		const UCombatAttributes* Attr = It->GetAttributes();
		const float Width = 154 * Scale;
		const float X = Screen.X - Width * .5f;
		Text(Screen.X, Screen.Y - 28 * Scale, FString::Printf(TEXT("%s   %.0f / %.0f"),
			It->GetAbilitySystemComponent()->HasMatchingGameplayTag(TrainingBurning) ? TEXT("训练靶 · 灼烧") : TEXT("训练靶"), Attr->GetHealth(), Attr->GetMaxHealth()), 14, true);
		DrawRect(FLinearColor(.02f,.035f,.05f,.9f), X, Screen.Y, Width, 7 * Scale);
		DrawRect(FLinearColor(.12f,.88f,.64f), X, Screen.Y, Width * Attr->GetHealth() / Attr->GetMaxHealth(), 7 * Scale);
	}
}
