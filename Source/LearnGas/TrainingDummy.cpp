#include "TrainingDummy.h"
#include "CombatAttributes.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "TrainingDefeatReward.h"

ATrainingDummy::ATrainingDummy()
{
	bReplicates = true;
	CreateDefaultSubobject<UTrainingDefeatReward>(TEXT("DefeatReward"));
	auto* Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TargetCollision"));
	SetRootComponent(Capsule);
	Capsule->InitCapsuleSize(42.f, 88.f);
	Capsule->SetCollisionProfileName(TEXT("Pawn"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	auto MakePart = [&](const TCHAR* Name, FVector Location, FRotator Rotation, FVector Scale)
	{
		auto* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(Cylinder.Object);
		Part->SetMaterial(0, Surface.Object);
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};
	MakePart(TEXT("Foot"), FVector(0,0,-82), FRotator::ZeroRotator, FVector(.76,.76,.12));
	MakePart(TEXT("Post"), FVector(0,0,-25), FRotator::ZeroRotator, FVector(.17,.17,1.05));
	const FRotator Face(90,0,0);
	Rim = MakePart(TEXT("TargetFace"), FVector(0,0,35), Face, FVector(.85,.85,.26));
	Ring = MakePart(TEXT("MiddleRing"), FVector(-14,0,35), Face, FVector(.61,.61,.04));
	Bullseye = MakePart(TEXT("Bullseye"), FVector(-17,0,35), Face, FVector(.30,.30,.04));

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	AbilitySystem->SetIsReplicated(true);
	AbilitySystem->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	Attributes = CreateDefaultSubobject<UCombatAttributes>(TEXT("Attributes"));
}

void ATrainingDummy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	auto Tint = [](UStaticMeshComponent* Part, FLinearColor Color)
	{
		if (auto* Material = Part->CreateAndSetMaterialInstanceDynamic(0)) Material->SetVectorParameterValue(TEXT("Color"), Color);
	};
	Tint(Rim, FLinearColor(.08f,.12f,.18f));
	Tint(Ring, FLinearColor(.85f,.88f,.80f));
	Tint(Bullseye, FLinearColor(.95f,.18f,.035f));
}

void ATrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	if (HasAuthority())
	{
		AbilitySystem->GetGameplayAttributeValueChangeDelegate(UCombatAttributes::GetHealthAttribute()).AddUObject(this, &ThisClass::HealthChanged);
	}
}

void ATrainingDummy::HealthChanged(const FOnAttributeChangeData& Change)
{
	if (Change.OldValue > 0.f && Change.NewValue <= 0.f)
	{
		AbilitySystem->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(TrainingBurning));
		GetWorldTimerManager().SetTimer(ResetTimer, this, &ThisClass::RestoreHealth, 2.f);
	}
}

void ATrainingDummy::RestoreHealth()
{
	AbilitySystem->SetNumericAttributeBase(UCombatAttributes::GetHealthAttribute(), Attributes->GetMaxHealth());
}
