#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "TrainingMode.generated.h"

class UCombatAttributes;
class UTrainingProgressionAttributes;
struct FOnAttributeChangeData;

UCLASS(Config=Game)
class LEARNGAS_API ATrainingPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	bool HasSkill(FGameplayTag Id) const;
	bool CanLearn(FGameplayTag Id) const;
	bool IsWardEquipped() const;
	void RestorePassive();
	UFUNCTION(Server, Reliable) void ServerLearnSkill(FGameplayTag Id);
	UFUNCTION(Server, Reliable) void ServerToggleWard();

	ATrainingPlayerState();
	virtual void BeginPlay() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }

	UFUNCTION(Server, Reliable) void ServerSwitchWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool HasGauntlets() const { return bGauntlets; }
	float GetWeaponMultiplier() const { return bGauntlets ? FMath::Max(0.f, GauntletMultiplier) : 1.f; }
	void SaveCheckpoint();
	void LoadCheckpoint();
	void RestoreCheckpointOnEntry();
	FString GetCheckpointNotice() const;

private:
	UPROPERTY(Replicated) bool bGauntlets = false;
	UPROPERTY(Config) float GauntletMultiplier = 1.5f;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UCombatAttributes> Attributes;
	UPROPERTY() TObjectPtr<UTrainingProgressionAttributes> Progression;
	FTimerHandle StaminaRegen;
	FTimerHandle RespawnTimer;
	void OnHealthChanged(const FOnAttributeChangeData& Change);
	void RespawnTrainingPlayer();
	bool bCheckedInitialCheckpoint = false;
	FString CheckpointNotice;
	double NoticeEndsAt = 0.;
	void ShowCheckpointNotice(const TCHAR* Message);

	void OnStaminaChanged(const FOnAttributeChangeData& Change);
	void RegenerateStamina();
};

UCLASS()
class LEARNGAS_API ATrainingMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ATrainingMode();
};
