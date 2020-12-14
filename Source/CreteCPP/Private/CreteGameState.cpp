// Fill out your copyright notice in the Description page of Project Settings.


#include "CreteGameState.h"
#include "Net/UnrealNetwork.h"

void ACreteGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACreteGameState, LatestEvent);
	DOREPLIFETIME(ACreteGameState, MissionSuccess);
}

