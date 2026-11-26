#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "TrainingDamage.h"
#include "TrainingSentinel.generated.h"

class ATrainingCharacter;
class UCombatAttributes;
struct FOnAttributeChangeData;

UENUM()
enum class ETrainingSentinelPhase : uint8 { Ready, Windup, Swing, Down };

UCLASS()
class ATrainingSentinel : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ATrainingSentinel();
	virtual void BeginPlay() override;
	virtual void Tick(float Delta) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return ASC; }
	ETrainingSentinelPhase GetPhase() const { return Phase; }
	ATrainingCharacter* GetTarget() const { return Target.Get(); }
	void SetPhase(ETrainingSentinelPhase NewPhase);
	void FinishAttack();
	void SetChallenger(ATrainingCharacter* Player);
private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UAbilitySystemComponent> ASC;
	UPROPERTY() TObjectPtr<UCombatAttributes> Attributes;
	UPROPERTY(ReplicatedUsing=OnRep_Phase) ETrainingSentinelPhase Phase = ETrainingSentinelPhase::Ready;
	UFUNCTION() void OnRep_Phase();
	TWeakObjectPtr<ATrainingCharacter> Target;
	TWeakObjectPtr<ATrainingCharacter> Challenger;
	bool bInEncounter = false;
	FVector Home;
	double NextAttackTime = 0.;
	FTimerHandle ResetTimer;
	void HealthChanged(const FOnAttributeChangeData& Change);
	void ResetOpponent();
};

UCLASS()
class UTrainingSentinelDamage : public UTrainingDamageEffect
{
	GENERATED_BODY()
public:
	UTrainingSentinelDamage();
};

UCLASS()
class UTrainingSentinelAttack : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UTrainingSentinelAttack();
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, const FGameplayEventData* Event) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* Info,
		FGameplayAbilityActivationInfo Activation, bool bReplicate, bool bCancelled) override;
private:
	UPROPERTY() TSoftObjectPtr<UAnimMontage> AttackMontage;
	TWeakObjectPtr<ATrainingCharacter> Victim;
	UFUNCTION() void Swing();
	UFUNCTION() void ResolveHit();
};
