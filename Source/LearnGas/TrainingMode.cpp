#include "TrainingMode.h"
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
