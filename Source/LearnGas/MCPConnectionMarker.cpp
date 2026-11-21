#include "MCPConnectionMarker.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

AMCPConnectionMarker::AMCPConnectionMarker()
{
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Origin")));

	UStaticMeshComponent* Cube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cube"));
	Cube->SetupAttachment(RootComponent);
	Cube->SetRelativeLocation(FVector(0, 0, 50));
	Cube->SetRelativeRotation(FRotator(0, 25, 0));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Cube->SetStaticMesh(CubeMesh.Object);

	Label = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConnectionLabel"));
	Label->SetupAttachment(RootComponent);
	Label->SetRelativeLocation(FVector(0, 0, 145));
	Label->SetRelativeScale3D(FVector(0.25));
	Label->SetDrawSize(FVector2D(768, 144));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetCastShadow(false);
}

void AMCPConnectionMarker::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	if (!IsRunningDedicatedServer() && Label->IsRegistered())
	{
		Label->SetSlateWidget(
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::AsCultureInvariant(TEXT("连接mcp成功")))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 68))
				.ColorAndOpacity(FLinearColor::White)
				.ShadowOffset(FVector2D(3, 3))
				.ShadowColorAndOpacity(FLinearColor::Black)
			]);
	}
}
