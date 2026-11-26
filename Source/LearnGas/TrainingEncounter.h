#pragma once

#include "GameFramework/Actor.h"
#include "TrainingEncounter.generated.h"

class ATrainingCharacter;
class ATrainingSentinel;

UENUM()
enum class ETrainingEncounterPhase : uint8 { Idle, Fighting, Intermission, Cleared, Failed };

UCLASS()
class ATrainingEncounter : public AActor
{
	GENERATED_BODY()
public:
	ATrainingEncounter();
	void TryStart(ATrainingCharacter* Player);
	void AbandonFor(ATrainingCharacter* Player);
	FString GetPrompt() const;
	virtual void Tick(float Delta) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
	UPROPERTY(Replicated) ETrainingEncounterPhase Phase = ETrainingEncounterPhase::Idle;
	UPROPERTY(Replicated) int32 Wave = 0;
	UPROPERTY(Replicated) int32 Remaining = 0;
	TWeakObjectPtr<ATrainingCharacter> Challenger;
	TArray<TWeakObjectPtr<ATrainingSentinel>> Guards;
	double NextWaveTime = 0.;
	void SpawnWave();
	void Finish(bool bWon);
	void ClearGuards();
};
