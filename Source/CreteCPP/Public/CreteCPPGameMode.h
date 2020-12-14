// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameLiftServerSDK.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "CreteCPPGameMode.generated.h"

USTRUCT()
struct FStartGameSessionState
{
	GENERATED_BODY();

	UPROPERTY()
	bool Status;

	UPROPERTY()
	FString MatchmakingConfigurationArn;

	TMap<FString, Aws::GameLift::Server::Model::Player> PlayerIdToPlayer;

	FStartGameSessionState() {
		Status = false;
	}
};

USTRUCT()
struct FUpdateGameSessionState
{
	GENERATED_BODY();

	FUpdateGameSessionState() {
		
	}
};

USTRUCT()
struct FProcessTerminateState
{
	GENERATED_BODY();


	UPROPERTY()
	bool Status;

	long TerminationTime;

	FProcessTerminateState() {
		Status = false;
		TerminationTime = 0L;
	}
};

USTRUCT()
struct FHealthCheckState
{
	GENERATED_BODY();


	UPROPERTY()
	bool Status;

	FHealthCheckState() {
		Status = false;
	}
};

UCLASS(minimalapi)
class ACreteCPPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACreteCPPGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting) override;

public:
	FHttpModule* HttpModule;

	UPROPERTY()
	FTimerHandle CountDownUntilGameOverHandle;

	UPROPERTY()
	FTimerHandle EndGameHandle;

	UPROPERTY()
	FTimerHandle DetermineMatchOutcomeHandle;

	UPROPERTY()
	FTimerHandle HandleProcessTerminationHandle;

	UPROPERTY()
	FTimerHandle HandleGameSessionUpdateHandle;

protected:
	virtual void BeginPlay() override;

	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

private:
	UPROPERTY()
	FStartGameSessionState StartGameSessionState;

	UPROPERTY()
	FUpdateGameSessionState UpdateGameSessionState;

	UPROPERTY()
	FProcessTerminateState ProcessTerminateState;

	UPROPERTY()
	FHealthCheckState HealthCheckState;

	UPROPERTY()
	FString ApiUrl;

	UPROPERTY()
	FString ServerPassword;

	UPROPERTY()
	int RemainingGameTime;

	UPROPERTY()
	bool GameSessionActivated;

	UFUNCTION()
	void CountDownUntilGameOver();

	UFUNCTION()
	void EndGame();

	UFUNCTION()
	void DetermineMatchOutcome();

	UFUNCTION()
	void HandleGameSessionUpdate();

	UFUNCTION()
	void HandleProcessTermination();

	void OnRecordMatchResultResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
};



