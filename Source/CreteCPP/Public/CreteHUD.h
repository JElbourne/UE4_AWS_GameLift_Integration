// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "CreteHUD.generated.h"

class UUserWidget;

/**
 * 
 */
UCLASS()
class CRETECPP_API ACreteHUD : public AHUD
{
	GENERATED_BODY()

public:
	ACreteHUD();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	TSubclassOf<UUserWidget> GameWidgetClass;
	
};
