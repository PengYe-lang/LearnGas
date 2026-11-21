#pragma once

#include "GameFramework/Actor.h"
#include "MCPConnectionMarker.generated.h"

class UWidgetComponent;

UCLASS()
class LEARNGAS_API AMCPConnectionMarker : public AActor
{
	GENERATED_BODY()

public:
	AMCPConnectionMarker();
	virtual void PostRegisterAllComponents() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Marker")
	TObjectPtr<UWidgetComponent> Label;
};
