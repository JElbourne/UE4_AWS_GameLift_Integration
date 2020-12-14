// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CretePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CRETECPP_API ACretePlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FString PlayerSessionId;

	UPROPERTY()
	FString MatchmakingPlayerId;

	UPROPERTY(Replicated)
	FString CharacterType;
	
};
