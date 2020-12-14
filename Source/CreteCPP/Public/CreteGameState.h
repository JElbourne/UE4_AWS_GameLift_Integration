// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CreteGameState.generated.h"

/**
 * 
 */
UCLASS()
class CRETECPP_API ACreteGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:

	UPROPERTY(Replicated)
	FString LatestEvent;

	UPROPERTY(Replicated)
	bool MissionSuccess;
};
