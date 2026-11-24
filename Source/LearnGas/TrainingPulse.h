#pragma once

#include "GameFramework/Actor.h"
#include "TrainingPulse.generated.h"

class ATrainingCharacter;
class UMaterialInstanceDynamic;

UENUM()
enum class ETrainingPulsePhase : uint8
{
	Idle,
	WindingUp,
	Cooldown
};

UCLASS()
class ATrainingPulse : public AActor
{
	GENERATED_BODY()
public:
	ATrainingPulse();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	bool IsWindingUp() const { return Phase == ETrainingPulsePhase::WindingUp; }

private:
	static constexpr float Radius = 140.f;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Marker;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MarkerMaterial;
	UPROPERTY(ReplicatedUsing=OnRep_Phase) ETrainingPulsePhase Phase = ETrainingPulsePhase::Idle;
	double PhaseEndsAt = 0.0;

	UFUNCTION() void OnRep_Phase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	bool IsInRange(const ATrainingCharacter& Character) const;
	void SetPhase(ETrainingPulsePhase NewPhase, double NextDelay);
};
