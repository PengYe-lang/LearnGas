#include "TrainingArmorZone.h"
#include "CombatAttributes.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrainingCharacter.h"
#include "TimerManager.h"

UTrainingArmorBuff::UTrainingArmorBuff()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	auto& Bonus = Modifiers.AddDefaulted_GetRef();
	Bonus.ModifierOp = EGameplayModOp::Additive;
	Bonus.Attribute = UCombatAttributes::GetArmorAttribute();
	Bonus.ModifierMagnitude = FScalableFloat(25.f);
	auto& Resistance = Modifiers.AddDefaulted_GetRef();
	Resistance.Attribute = UCombatAttributes::GetFireResistanceAttribute();
	Resistance.ModifierOp = EGameplayModOp::Additive;
	Resistance.ModifierMagnitude = FScalableFloat(20.f);
}

ATrainingArmorZone::ATrainingArmorZone()
{
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	SetRootComponent(Area);
	Area->InitBoxExtent(FVector(125, 115, 110));
	Area->SetCollisionProfileName(TEXT("Trigger"));

	auto* Tile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmorTile"));
	Tile->SetupAttachment(Area);
	Tile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tile->SetRelativeScale3D(FVector(2.5, 2.3, .025));
	Tile->SetRelativeLocation(FVector(0, 0, -108));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
	Tile->SetStaticMesh(Cube.Object);
	auto* Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
	Label->SetupAttachment(Area);
	Label->SetRelativeLocationAndRotation(FVector(100, 0, -60), FRotator(0,180,0));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetText(FText::FromString(TEXT("ARMOR +25")));
	Label->SetWorldSize(22);
}

void ATrainingArmorZone::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Tile = FindComponentByClass<UStaticMeshComponent>())
	{
		if (auto* Material = Tile->CreateAndSetMaterialInstanceDynamic(0))
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(.08f,.45f,.62f));
	}
	if (HasAuthority())
	{
		RefreshOccupants();
		GetWorldTimerManager().SetTimer(RefreshTimer, this, &ThisClass::RefreshOccupants, .10f, true);
	}
}

void ATrainingArmorZone::RefreshOccupants()
{
	TArray<AActor*> Occupants;
	Area->GetOverlappingActors(Occupants, ATrainingCharacter::StaticClass());
	TSet<TWeakObjectPtr<UAbilitySystemComponent>> Present;
	for (AActor* Actor : Occupants)
	{
		auto* ASC = CastChecked<ATrainingCharacter>(Actor)->GetAbilitySystemComponent();
		if (!ASC || ASC->GetAvatarActor() != Actor) continue;
		Present.Add(ASC);
		if (Grants.Contains(ASC)) continue;
		auto Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		const auto Spec = ASC->MakeOutgoingSpec(UTrainingArmorBuff::StaticClass(), 1, Context);
		if (!Spec.IsValid()) continue;
		const auto Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data);
		if (Handle.IsValid()) Grants.Add(ASC, Handle);
	}
	for (auto It = Grants.CreateIterator(); It; ++It)
	{
		if (Present.Contains(It.Key())) continue;
		if (It.Key().IsValid()) It.Key()->RemoveActiveGameplayEffect(It.Value());
		It.RemoveCurrent();
	}
}

void ATrainingArmorZone::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(RefreshTimer);
	for (const auto& Grant : Grants)
		if (Grant.Key.IsValid()) Grant.Key->RemoveActiveGameplayEffect(Grant.Value);
	Grants.Empty();
	Super::EndPlay(Reason);
}
