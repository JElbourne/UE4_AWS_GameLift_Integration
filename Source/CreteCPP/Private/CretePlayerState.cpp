// Fill out your copyright notice in the Description page of Project Settings.


#include "CretePlayerState.h"
#include "Net/UnrealNetwork.h"

void ACretePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACretePlayerState, CharacterType);
}
