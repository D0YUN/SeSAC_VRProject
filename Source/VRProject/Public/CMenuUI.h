#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CMenuUI.generated.h"

UCLASS()
class VRPROJECT_API UCMenuUI : public UUserWidget
{
	GENERATED_BODY()
	
	// ¾Û Á¾·á
public:
	UFUNCTION(BlueprintCallable, Category = "MenuEvent")
	void QuitVRGame();
};
