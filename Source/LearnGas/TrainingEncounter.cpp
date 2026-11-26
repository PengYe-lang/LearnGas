#include "TrainingEncounter.h"
#include "CombatAttributes.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TrainingCharacter.h"
#include "TrainingMode.h"
#include "TrainingSentinel.h"

ATrainingEncounter::ATrainingEncounter()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .15f;
	auto* Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrialPad"));
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));
	Pad->SetupAttachment(RootComponent);
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
	Pad->SetStaticMesh(Cylinder.Object);
	Pad->SetRelativeScale3D(FVector(2.7f,2.7f,.04f));
}

void ATrainingEncounter::TryStart(ATrainingCharacter* Player)
{
	if (!HasAuthority() || !Player || !Player->GetAbilitySystemComponent()
		|| Player->GetAbilitySystemComponent()->GetNumericAttribute(UCombatAttributes::GetHealthAttribute()) <= 0.f
		|| FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(235.f)
		|| Phase == ETrainingEncounterPhase::Fighting || Phase == ETrainingEncounterPhase::Intermission) return;
	Challenger = Player;
	Wave = 0;
	SpawnWave();
}

void ATrainingEncounter::AbandonFor(ATrainingCharacter* Player)
{
	if (HasAuthority() && Player && Challenger.Get() == Player) Finish(false);
}

void ATrainingEncounter::SpawnWave()
{
	++Wave;
	Phase = ETrainingEncounterPhase::Fighting;
	for (int32 Index = 0; Index < Wave; ++Index)
	{
		const float Offset = Wave == 1 ? 0.f : (Index == 0 ? -120.f : 120.f);
		const FTransform Spawn(FRotator(0,180,0), GetActorLocation() + FVector(365, Offset, 92));
		auto* Guard = GetWorld()->SpawnActorDeferred<ATrainingSentinel>(ATrainingSentinel::StaticClass(), Spawn,
			this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
		if (!Guard)
		{
			Finish(false);
			return;
		}
		Guard->SetChallenger(Challenger.Get());
		Guard->FinishSpawning(Spawn);
		if (!IsValid(Guard) || Guard->IsActorBeingDestroyed())
		{
			Finish(false);
			return;
		}
		Guards.Add(Guard);
	}
	Remaining = Wave;
	ForceNetUpdate();
}

void ATrainingEncounter::Tick(float Delta)
{
	Super::Tick(Delta);
	if (!HasAuthority() || (Phase != ETrainingEncounterPhase::Fighting && Phase != ETrainingEncounterPhase::Intermission)) return;
	auto* Player = Challenger.Get();
	auto* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (!ASC || ASC->GetNumericAttribute(UCombatAttributes::GetHealthAttribute()) <= 0.f
		|| FVector::DistSquared(GetActorLocation(), Player->GetActorLocation()) > FMath::Square(850.f))
	{
		Finish(false);
		return;
	}
	if (Phase == ETrainingEncounterPhase::Intermission)
	{
		if (GetWorld()->GetTimeSeconds() >= NextWaveTime) SpawnWave();
		return;
	}
	Remaining = 0;
	for (const auto& Guard : Guards)
	{
		if (!Guard.IsValid()) { Finish(false); return; }
		if (Guard->GetPhase() != ETrainingSentinelPhase::Down) ++Remaining;
	}
	if (Remaining > 0) return;
	ClearGuards();
	if (Wave >= 2) { Finish(true); return; }
	Phase = ETrainingEncounterPhase::Intermission;
	NextWaveTime = GetWorld()->GetTimeSeconds() + 2.;
	ForceNetUpdate();
}

void ATrainingEncounter::ClearGuards()
{
	for (const auto& Guard : Guards)
		if (Guard.IsValid()) Guard->Destroy();
	Guards.Reset();
	Remaining = 0;
}

void ATrainingEncounter::Finish(bool bWon)
{
	ClearGuards();
	Phase = bWon ? ETrainingEncounterPhase::Cleared : ETrainingEncounterPhase::Failed;
	if (bWon && Challenger.IsValid())
		if (auto* State = Challenger->GetPlayerState<ATrainingPlayerState>()) State->RecordTrialClear();
	Challenger.Reset();
	ForceNetUpdate();
}

void ATrainingEncounter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority()) ClearGuards();
	Super::EndPlay(Reason);
}

FString ATrainingEncounter::GetPrompt() const
{
	switch (Phase)
	{
	case ETrainingEncounterPhase::Fighting: return FString::Printf(TEXT("守卫试炼 · 第 %d / 2 波 · 剩 %d 人"), Wave, Remaining);
	case ETrainingEncounterPhase::Intermission: return TEXT("第一波已击破 · 第二波即将入场");
	case ETrainingEncounterPhase::Cleared: return TEXT("守卫试炼已通关 · E 再挑战");
	case ETrainingEncounterPhase::Failed: return TEXT("试炼结束 · 返回圆台按 E 重试");
	default: return TEXT("守卫试炼 · 到圆台按 E 开始");
	}
}

void ATrainingEncounter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrainingEncounter, Phase);
	DOREPLIFETIME(ATrainingEncounter, Wave);
	DOREPLIFETIME(ATrainingEncounter, Remaining);
}
