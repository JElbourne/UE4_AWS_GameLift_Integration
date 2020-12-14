// Fill out your copyright notice in the Description page of Project Settings.


#include "CreteHUD.h"
#include "Blueprint/UserWidget.h"

ACreteHUD::ACreteHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> GameObj(TEXT("/Game/_Crete/_Production/UI/Widgets/UI_Game"));
	GameWidgetClass = GameObj.Class;
}

void ACreteHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (PlayerController != nullptr)
	{
		PlayerController->bShowMouseCursor = false;
	}

	if (GameWidgetClass != nullptr)
	{
		UUserWidget* GameWidget = CreateWidget<UUserWidget>(GetWorld(), GameWidgetClass);
		if(GameWidget != nullptr)
		{
			GameWidget->AddToViewport();
		}
	}
}
