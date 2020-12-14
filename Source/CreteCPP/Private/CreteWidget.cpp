// Fill out your copyright notice in the Description page of Project Settings.


#include "CreteWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/GameStateBase.h"
#include "CretePlayerState.h"
#include "CreteGameState.h"
#include "CreteGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UCreteWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TeammateCountTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_TeammateCount"));
	EventTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Event"));
	PingTextBlock = (UTextBlock*)GetWidgetFromName(TEXT("TextBlock_Ping"));

	GetWorld()->GetTimerManager().SetTimer(SetTeammateCountHandle, this, &UCreteWidget::SetTeammateCount, 1.0f, true, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(SetLatestEventHandle, this, &UCreteWidget::SetLatestEvent, 1.0f, true, 1.0f);
	GetWorld()->GetTimerManager().SetTimer(SetAveragePlayerLatencyHandle, this, &UCreteWidget::SetAveragePlayerLatency, 1.0f, true, 1.0f);
}

void UCreteWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(SetTeammateCountHandle);
	GetWorld()->GetTimerManager().ClearTimer(SetLatestEventHandle);
	GetWorld()->GetTimerManager().ClearTimer(SetAveragePlayerLatencyHandle);

	Super::NativeDestruct();
}

void UCreteWidget::SetTeammateCount()
{
	APlayerState* OwningPlayerState = GetOwningPlayerState();

	if (OwningPlayerState != nullptr)
	{
		ACretePlayerState* OwningCretePlayerState = Cast<ACretePlayerState>(OwningPlayerState);
		if (OwningCretePlayerState != nullptr)
		{
			// TODO if w
		}
	}

	TArray<APlayerState*> PlayerStates = GetWorld()->GetGameState()->PlayerArray;

	int TeammateCount = PlayerStates.Num();
	TeammateCountTextBlock->SetText(FText::FromString("Teammate Count: " + FString::FromInt(TeammateCount)));
}

void UCreteWidget::SetLatestEvent()
{
	FString LatestEvent;
	bool MissionSuccess = false;

	AGameStateBase* GameState = GetWorld()->GetGameState();
	
	if (GameState != nullptr)
	{
		ACreteGameState* CreteGameState = Cast<ACreteGameState>(GameState);
		if (CreteGameState != nullptr)
		{
			LatestEvent = CreteGameState->LatestEvent;
			MissionSuccess = CreteGameState->MissionSuccess;
		}

		if (LatestEvent.Len() > 0)
		{
			if (LatestEvent.Equals("GameEnded"))
			{
				FString ResultText = MissionSuccess ? "Won" : "Lost";
				EventTextBlock->SetText(FText::FromString("The Humans " + ResultText));
			}
			else
			{
				EventTextBlock->SetText(FText::FromString(LatestEvent));
			}
		}
	}
}

void UCreteWidget::SetAveragePlayerLatency()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance != nullptr)
	{
		UCreteGameInstance* CreteGameInstance = Cast<UCreteGameInstance>(GameInstance);
		if (CreteGameInstance != nullptr)
		{
			float TotalPlayerLatency = 0.f;
			for (float PlayerLatency : CreteGameInstance->PlayerLatencies)
			{
				TotalPlayerLatency += PlayerLatency;
			}

			float AveragePlayerLatency = 60.0f;;

			if (TotalPlayerLatency > 0)
			{
				AveragePlayerLatency = TotalPlayerLatency / CreteGameInstance->PlayerLatencies.Num();

				FString PingString = "Ping: " + FString::FromInt(FMath::RoundToInt(AveragePlayerLatency)) + "ms";
				PingTextBlock->SetText(FText::FromString(PingString));
			}
		}
	}
}
