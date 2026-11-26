#include "TrainingMode.h"
#include "TrainingCheckpoint.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CombatAttributes.h"
#include "LightStrikeAbility.h"
#include "TrainingCharacter.h"
#include "TrainingHUD.h"
#include "TrainingProgressionAttributes.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

void ATrainingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrainingPlayerState, bGauntlets);
}

void ATrainingPlayerState::ServerSwitchWeapon_Implementation()
{
	const auto* Character = Cast<ATrainingCharacter>(AbilitySystem->GetAvatarActor());
	if (!Character || Character->ActiveStrike.IsValid() || AbilitySystem->HasMatchingGameplayTag(TrainingDodging)
		|| AbilitySystem->HasMatchingGameplayTag(TrainingHitStun)) return;
	bGauntlets = !bGauntlets;
}

ATrainingPlayerState::ATrainingPlayerState()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	Attributes = CreateDefaultSubobject<UCombatAttributes>(TEXT("Attributes"));
	Progression = CreateDefaultSubobject<UTrainingProgressionAttributes>(TEXT("Progression"));
	SetNetUpdateFrequency(30.f);
}

ATrainingMode::ATrainingMode()
{
	DefaultPawnClass = ATrainingCharacter::StaticClass();
	PlayerStateClass = ATrainingPlayerState::StaticClass();
	HUDClass = ATrainingHUD::StaticClass();
}

void ATrainingPlayerState::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributes::GetStaminaAttribute())
			.AddUObject(this, &ThisClass::OnStaminaChanged);
	}
}

void ATrainingPlayerState::OnStaminaChanged(const FOnAttributeChangeData& Change)
{
	if (Change.NewValue < Change.OldValue)
	{
		GetWorldTimerManager().SetTimer(StaminaRegen, this, &ThisClass::RegenerateStamina, .10f, true, 1.30f);
	}
}

bool ATrainingPlayerState::HasLearnedHeavyStrike() const
{
	return AbilitySystem->FindAbilitySpecFromClass(UHeavyStrikeAbility::StaticClass()) != nullptr;
}

bool ATrainingPlayerState::CanLearnHeavyStrike() const
{
	return !HasLearnedHeavyStrike()
		&& Progression->GetLevel() >= HeavyStrikeRequiredLevel
		&& Progression->GetSkillPoints() >= HeavyStrikeSkillPointCost;
}

void ATrainingPlayerState::ServerLearnHeavyStrike_Implementation()
{
	if (!HasAuthority() || !CanLearnHeavyStrike()) return;

	const FGameplayAbilitySpecHandle Learned = AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHeavyStrikeAbility::StaticClass(), 1));
	if (Learned.IsValid())
	{
		Progression->SetSkillPoints(Progression->GetSkillPoints() - HeavyStrikeSkillPointCost);
	}
}

void ATrainingPlayerState::RegenerateStamina()
{
	const float Stamina = FMath::Min(Attributes->GetStamina() + 2.4f, UCombatAttributes::StaminaCapacity);
	AbilitySystem->SetNumericAttributeBase(UCombatAttributes::GetStaminaAttribute(), Stamina);
	if (Stamina >= UCombatAttributes::StaminaCapacity) GetWorldTimerManager().ClearTimer(StaminaRegen);
}

void ATrainingPlayerState::ShowCheckpointNotice(const TCHAR* Message)
{
	CheckpointNotice = Message;
	NoticeEndsAt = GetWorld()->GetTimeSeconds() + 3.;
}

FString ATrainingPlayerState::GetCheckpointNotice() const
{
	return GetWorld()->GetTimeSeconds() < NoticeEndsAt ? CheckpointNotice : FString();
}

void ATrainingPlayerState::SaveCheckpoint()
{
	if (GetNetMode() != NM_Standalone) return;
	auto* Save = NewObject<UTrainingCheckpoint>(this);
	Save->Experience = FMath::RoundToInt(Progression->GetExperience());
	Save->SkillPoints = FMath::RoundToInt(Progression->GetSkillPoints());
	Save->bLearnedHeavy = HasLearnedHeavyStrike();
	Save->bGauntlets = bGauntlets;
	Save->Health = Attributes->GetHealth();
	Save->Stamina = Attributes->GetStamina();
	ShowCheckpointNotice(Save->WriteNext() ? TEXT("训练进度已保存") : TEXT("训练进度保存失败"));
}

void ATrainingPlayerState::RestoreCheckpointOnEntry()
{
	if (GetNetMode() != NM_Standalone || bCheckedInitialCheckpoint) return;
	bCheckedInitialCheckpoint = true;
	LoadCheckpoint();
}

void ATrainingPlayerState::LoadCheckpoint()
{
	if (GetNetMode() != NM_Standalone) return;
	int32 SlotIndex;
	const auto* Save = UTrainingCheckpoint::LoadLatest(SlotIndex);
	if (!Save)
	{
		ShowCheckpointNotice(TEXT("没有可读取的训练存档"));
		return;
	}

	AbilitySystem->CancelAbilities();
	AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TrainingHitStun));
	if (auto* Character = Cast<ATrainingCharacter>(AbilitySystem->GetAvatarActor()))
	{
		Character->StopAnimMontage();
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
	AbilitySystem->SetNumericAttributeBase(UTrainingProgressionAttributes::GetExperienceAttribute(), Save->Experience);
	Progression->SetLevel(1 + Save->Experience / UTrainingProgressionAttributes::ExperiencePerLevel);
	Progression->SetSkillPoints(Save->SkillPoints);
	bGauntlets = Save->bGauntlets;
	auto* Heavy = AbilitySystem->FindAbilitySpecFromClass(UHeavyStrikeAbility::StaticClass());
	if (Save->bLearnedHeavy && !Heavy)
	{
		AbilitySystem->GiveAbility(FGameplayAbilitySpec(UHeavyStrikeAbility::StaticClass(), 1));
	}
	else if (!Save->bLearnedHeavy && Heavy)
	{
		AbilitySystem->ClearAbility(Heavy->Handle);
	}
	Attributes->SetHealth(FMath::Min(Save->Health, Attributes->GetMaxHealth()));
	Attributes->SetStamina(Save->Stamina);
	GetWorldTimerManager().ClearTimer(StaminaRegen);
	if (Save->Stamina < UCombatAttributes::StaminaCapacity)
	{
		GetWorldTimerManager().SetTimer(StaminaRegen, this, &ThisClass::RegenerateStamina, .10f, true, 1.30f);
	}
	ShowCheckpointNotice(TEXT("训练进度已恢复"));
}
