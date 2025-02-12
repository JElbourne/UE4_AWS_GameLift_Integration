// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreteWidget.generated.h"

class UTextBlock;
/**
 * 
 */
UCLASS()
class CRETECPP_API UCreteWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY()
	FTimerHandle SetTeammateCountHandle;

	UPROPERTY()
	FTimerHandle SetLatestEventHandle;

	UPROPERTY()
	FTimerHandle SetAveragePlayerLatencyHandle;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UPROPERTY()
	UTextBlock* TeammateCountTextBlock;

	UPROPERTY()
	UTextBlock* EventTextBlock;

	UPROPERTY()
	UTextBlock* PingTextBlock;

	UFUNCTION()
	void SetTeammateCount();

	UFUNCTION()
	void SetLatestEvent();

	UFUNCTION()
	void SetAveragePlayerLatency();
};
