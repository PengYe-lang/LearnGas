#include "TrainingPulse.h"

#include "CombatAttributes.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TrainingCharacter.h"

ATrainingPulse::ATrainingPulse()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .04f;
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PulseArea"));
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));
	Marker->SetupAttachment(RootComponent);
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Marker->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Disc(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	Marker->SetStaticMesh(Disc.Object);
	Marker->SetMaterial(0, Surface.Object);
	Marker->SetRelativeScale3D(FVector(Radius / 50.f, Radius / 50.f, .025f));
}

void ATrainingPulse::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	MarkerMaterial = Marker->CreateAndSetMaterialInstanceDynamic(0);
	OnRep_Phase();
}

void ATrainingPulse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || GetWorld()->GetTimeSeconds() < PhaseEndsAt) return;

	if (Phase == ETrainingPulsePhase::Cooldown)
	{
		SetPhase(ETrainingPulsePhase::Idle, 0.0);
		return;
	}

	for (TActorIterator<ATrainingCharacter> It(GetWorld()); It; ++It)
	{
		if (!IsInRange(**It)) continue;
		if (Phase == ETrainingPulsePhase::Idle)
		{
			SetPhase(ETrainingPulsePhase::WindingUp, .80);
			return;
		}

		if (auto* ASC = It->GetAbilitySystemComponent())
		{
			auto Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);
			const auto Damage = ASC->MakeOutgoingSpec(UTrainingPulseDamage::StaticClass(), 1.f, Context);
			if (Damage.IsValid()) ASC->ApplyGameplayEffectSpecToSelf(*Damage.Data);
		}
	}

	if (IsWindingUp()) SetPhase(ETrainingPulsePhase::Cooldown, 1.30);
}

bool ATrainingPulse::IsInRange(const ATrainingCharacter& Character) const
{
	const FVector Delta = Character.GetActorLocation() - GetActorLocation();
	return FMath::Abs(Delta.Z) < 150.f && Delta.SizeSquared2D() <= FMath::Square(Radius);
}

void ATrainingPulse::SetPhase(ETrainingPulsePhase NewPhase, double NextDelay)
{
	Phase = NewPhase;
	PhaseEndsAt = GetWorld()->GetTimeSeconds() + NextDelay;
	ForceNetUpdate();
	OnRep_Phase();
}

void ATrainingPulse::OnRep_Phase()
{
	if (!MarkerMaterial) return;
	const FLinearColor Tint = IsWindingUp() ? FLinearColor(1.f, .42f, .06f)
		: Phase == ETrainingPulsePhase::Cooldown ? FLinearColor(.85f, .09f, .05f)
		: FLinearColor(.40f, .18f, .08f);
	MarkerMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
}

void ATrainingPulse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ATrainingPulse, Phase);
}
