// Copyright Epic Games, Inc. All Rights Reserved.

#include "CreteCPPGameMode.h"
#include "CreteCPPCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "CreteHud.h"
#include "CretePlayerState.h"
#include "CreteGameState.h"
#include "TextReaderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"

ACreteCPPGameMode::ACreteCPPGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
		HUDClass = ACreteHUD::StaticClass();
		PlayerStateClass = ACretePlayerState::StaticClass();
		GameStateClass = ACreteGameState::StaticClass();
	}

	UTextReaderComponent* TextReader = CreateDefaultSubobject<UTextReaderComponent>(TEXT("TextReaderComp"));
	ApiUrl = TextReader->ReadFile("Urls/ApiUrl.txt");

	HttpModule = &FHttpModule::Get();

	RemainingGameTime = 240;
	GameSessionActivated = false;
}

void ACreteCPPGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

#if WITH_GAMELIFT
	if (Options.Len() > 0)
	{
		const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
		const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

		if (PlayerSessionId.Len() > 0 && PlayerId.Len() > 0)
		{
			Aws::GameLift::Server::Model::DescribePlayerSessionsRequest DescribePlayerSessionsRequest;
			DescribePlayerSessionsRequest.SetPlayerSessionId(TCHAR_TO_ANSI(*PlayerSessionId));

			auto DescribePlayerSessionsOutcome = Aws::GameLift::Server::DescribePlayerSessions(DescribePlayerSessionsRequest);
			if (DescribePlayerSessionsOutcome.IsSuccess())
			{
				auto DescribePlayerSessionResult = DescribePlayerSessionsOutcome.GetResult();
				int Count = 1;
				auto PlayerSessions = DescribePlayerSessionResult.GetPlayerSessions(Count);
				if (PlayerSessions != nullptr)
				{
					auto PlayerSession = PlayerSessions[0];
					FString ExpectedPlayerId = PlayerSession.GetPlayerId();
					auto PlayerStatus = PlayerSession.GetStatus();

					if (ExpectedPlayerId.Equals(PlayerId) && PlayerStatus == Aws::GameLift::Server::Model::PlayerSessionStatus::RESERVED)
					{
						auto AcceptPlayerSessionOutcome = Aws::GameLift::Server::AcceptPlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));

						if (!AcceptPlayerSessionOutcome.IsSuccess())
						{
							ErrorMessage = "Unauthorized";
						}
					}
					else
					{
						ErrorMessage = "Unauthorized";
					}
				}
				else
				{
					ErrorMessage = "Unauthorized";
				}
			}
			else
			{
				ErrorMessage = "Unauthorized";
			}
		}
		else
		{
			ErrorMessage = "Unauthorized";
		}
	}
	else
	{
		ErrorMessage = "Unauthorized";
	}
#endif
}

void ACreteCPPGameMode::Logout(AController* Exiting)
{
#if WITH_GAMELIFT
	if (Exiting != nullptr)
	{
		APlayerState* PlayerState = Exiting->PlayerState;
		if (PlayerState != nullptr)
		{
			ACretePlayerState* CretePlayerState = Cast<ACretePlayerState>(PlayerState);
			const FString& PlayerSessionId = CretePlayerState->PlayerSessionId;
			if (PlayerSessionId.Len() > 0)
			{
				Aws::GameLift::Server::RemovePlayerSession(TCHAR_TO_ANSI(*PlayerSessionId));
			}
		}
	}
#endif
	Super::Logout(Exiting);
}

void ACreteCPPGameMode::BeginPlay()
{
	Super::BeginPlay();

#if WITH_GAMELIFT
	auto InitSDKOutcome = Aws::GameLift::Server::InitSDK();

	if (InitSDKOutcome.IsSuccess())
	{
		auto OnStartGameSession = [](Aws::GameLift::Server::Model::GameSession GameSessionObj, void* Params)
		{
			FStartGameSessionState* State = (FStartGameSessionState*)Params;

			State->Status = Aws::GameLift::Server::ActivateGameSession().IsSuccess();

			FString MatchmakerData = GameSessionObj.GetMatchmakerData();

			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MatchmakerData);

			if (FJsonSerializer::Deserialize(Reader, JsonObject))
			{
				State->MatchmakingConfigurationArn = JsonObject->GetStringField("matchmakingConfigurationArn");

				TArray<TSharedPtr<FJsonValue>> Teams = JsonObject->GetArrayField("teams");
				for (TSharedPtr<FJsonValue> Team : Teams)
				{
					TSharedPtr<FJsonObject> TeamObj = Team->AsObject();
					FString TeamName = TeamObj->GetStringField("name");

					TArray<TSharedPtr<FJsonValue>> Players = TeamObj->GetArrayField("players");

					for (TSharedPtr<FJsonValue> Player : Players)
					{
						TSharedPtr<FJsonObject> PlayerObj = Player->AsObject();
						FString PlayerId = PlayerObj->GetStringField("playerId");

						TSharedPtr<FJsonObject> Attributes = PlayerObj->GetObjectField("attributes");
						TSharedPtr<FJsonObject> Skill = Attributes->GetObjectField("skill");
						FString SkillValue = Skill->GetStringField("valueAttribute");

						auto SkillAttributeValue = new Aws::GameLift::Server::Model::AttributeValue(FCString::Atod(*SkillValue));

						Aws::GameLift::Server::Model::Player AwsPlayerObj;

						AwsPlayerObj.SetPlayerId(TCHAR_TO_ANSI(*PlayerId));
						AwsPlayerObj.SetTeam(TCHAR_TO_ANSI(*TeamName));
						AwsPlayerObj.AddPlayerAttribute("skill", *SkillAttributeValue);

						State->PlayerIdToPlayer.Add(PlayerId, AwsPlayerObj);
					}
				}
			}
		};

		auto OnUpdateGameSession = [](Aws::GameLift::Server::Model::UpdateGameSession UpdateGameSessionObj, void* Params)
		{
			FUpdateGameSessionState* State = (FUpdateGameSessionState*)Params;
		};

		auto OnProcessTerminate = [](void* Params)
		{
			FProcessTerminateState* State = (FProcessTerminateState*)Params;
			auto GetTerminationTimeOutcome = Aws::GameLift::Server::GetTerminationTime();
			if (GetTerminationTimeOutcome.IsSuccess())
			{
				State->TerminationTime = GetTerminationTimeOutcome.GetResult();
			}

			State->Status = true;
		};

		auto OnHealthCheck = [](void* Params)
		{
			FHealthCheckState* State = (FHealthCheckState*)Params;
			State->Status = true;

			return State->Status;
		};

		TArray<FString> CommandLineTokens;
		TArray<FString> CommandLineSwitches;
		int Port = FURL::UrlConfig.DefaultPort;

		FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

		for (FString Str : CommandLineSwitches)
		{
			FString Key;
			FString Value;

			if (Str.Split("=", &Key, &Value))
			{
				if (Key.Equals("port"))
				{
					Port = FCString::Atoi(*Value);
				}
				else if (Key.Equals("password")) {
					ServerPassword = Value;
				}
			}
		}

		const char* LogFile = "aLogFile.txt";
		const char** LogFiles = &LogFile;
		auto LogParams = new Aws::GameLift::Server::LogParameters(LogFiles, 1);

		auto Params = new Aws::GameLift::Server::ProcessParameters(
			OnStartGameSession,
			&StartGameSessionState,
			OnUpdateGameSession,
			&UpdateGameSessionState,
			OnProcessTerminate,
			&ProcessTerminateState,
			OnHealthCheck,
			&HealthCheckState,
			Port,
			*LogParams
		);

		auto ProcessReadyOutcome = Aws::GameLift::Server::ProcessReady(*Params);
	}
#endif

	GetWorldTimerManager().SetTimer(HandleGameSessionUpdateHandle, this, &ACreteCPPGameMode::HandleGameSessionUpdate, 1.0f, true, 5.0f);
	GetWorldTimerManager().SetTimer(HandleProcessTerminationHandle, this, &ACreteCPPGameMode::HandleProcessTermination, 1.0f, true, 5.0f);

	/*if (GameState != nullptr)
	{
		ACreteGameState* CreteGameState = Cast<ACreteGameState>(GameState);
		if (CreteGameState != nullptr)
		{
			CreteGameState->LatestEvent = "GameEnded";
			CreteGameState->MatchResult = "Win";
		}
	}*/
}




FString ACreteCPPGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString InitializedString = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	/*if (NewPlayerController != nullptr)
	{
		APlayerState* PlayerState = NewPlayerController->PlayerState;
		if (PlayerState != nullptr)
		{
			ACretePlayerState* CretePlayerState = Cast<ACretePlayerState>(PlayerState);
			if (CretePlayerState != nullptr)
			{
				if (FMath::RandRange(0, 1) == 0)
				{
					CretePlayerState->CharacterType = "Range";
				}
				else
				{
					CretePlayerState->CharacterType = "Melee";
				}
			}
		}
	}*/

#if WITH_GAMELIFT
	const FString& PlayerSessionId = UGameplayStatics::ParseOption(Options, "PlayerSessionId");
	const FString& PlayerId = UGameplayStatics::ParseOption(Options, "PlayerId");

	if (NewPlayerController != nullptr)
	{
		APlayerState* PlayerState = NewPlayerController->PlayerState;
		if (PlayerState != nullptr)
		{
			ACretePlayerState* CretePlayerState = Cast<ACretePlayerState>(PlayerState);
			if (CretePlayerState != nullptr)
			{
				CretePlayerState->PlayerSessionId = *PlayerSessionId;
				CretePlayerState->MatchmakingPlayerId = *PlayerId;

				if(StartGameSessionState.PlayerIdToPlayer.Num() > 0)
				{
					if (StartGameSessionState.PlayerIdToPlayer.Contains(PlayerId))
					{
						auto PlayerObj = StartGameSessionState.PlayerIdToPlayer.Find(PlayerId);
						FString Team = PlayerObj->GetTeam();
						CretePlayerState->CharacterType = *Team;
					}
				}
			}
		}
	}
#endif
	return InitializedString;

}

void ACreteCPPGameMode::CountDownUntilGameOver()
{
	if (GameState != nullptr)
	{
		ACreteGameState* CreteGameState = Cast<ACreteGameState>(GameState);
		if (CreteGameState != nullptr)
		{
			CreteGameState->LatestEvent = FString::FromInt(RemainingGameTime) + " seconds until game is over";
		}
	}

	if (RemainingGameTime > 0)
	{
		RemainingGameTime--;
	}
	else
	{
		GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	}
}

void ACreteCPPGameMode::EndGame()
{
	GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
	GetWorldTimerManager().ClearTimer(EndGameHandle);
	GetWorldTimerManager().ClearTimer(HandleProcessTerminationHandle);
	GetWorldTimerManager().ClearTimer(HandleGameSessionUpdateHandle);
	GetWorldTimerManager().ClearTimer(DetermineMatchOutcomeHandle);

#if WITH_GAMELIFT
	Aws::GameLift::Server::TerminateGameSession();
	Aws::GameLift::Server::ProcessEnding();
	FGenericPlatformMisc::RequestExit(false);
#endif
}

void ACreteCPPGameMode::DetermineMatchOutcome()
{
	GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
#if WITH_GAMELIFT
	if (GameState != nullptr)
	{
		ACreteGameState* CreteGameState = Cast<ACreteGameState>(GameState);
		if (CreteGameState != nullptr)
		{
			CreteGameState->LatestEvent = "Game Ended";

			if (FMath::RandRange(0, 1) == 0)
			{
				CreteGameState->MissionSuccess = true;
			}
			else
			{
				CreteGameState->MissionSuccess = false;
			}

			TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
			RequestObj->SetBoolField("missionSuccess", CreteGameState->MissionSuccess);

			auto GetGameSessionIdOutcome = Aws::GameLift::Server::GetGameSessionId();
			if (GetGameSessionIdOutcome.IsSuccess())
			{
				RequestObj->SetStringField("gameSessionId", GetGameSessionIdOutcome.GetResult());

				FString RequestBody;
				TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

				if (FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer))
				{
					TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RecordMatchResultRequest = HttpModule->CreateRequest();
					RecordMatchResultRequest->OnProcessRequestComplete().BindUObject(this, &ACreteCPPGameMode::OnRecordMatchResultResponseReceived);
					RecordMatchResultRequest->SetURL(ApiUrl + "/recordmatchresult");
					RecordMatchResultRequest->SetVerb("POST");
					RecordMatchResultRequest->SetHeader("Authorization", ServerPassword);
					RecordMatchResultRequest->SetHeader("Content-Type", "application/json");
					RecordMatchResultRequest->SetContentAsString(RequestBody);
					RecordMatchResultRequest->ProcessRequest();
				}
				else
				{
					GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 5.0f);
				}
			}
			else
			{
				GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 5.0f);
			}
		}
		else
		{
			GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 5.0f);
		}
	}
	else
	{
		GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 5.0f);
	}
#endif
}

void ACreteCPPGameMode::HandleGameSessionUpdate()
{
	if (!GameSessionActivated)
	{
		if (StartGameSessionState.Status)
		{
			GameSessionActivated = true;

			GetWorldTimerManager().SetTimer(DetermineMatchOutcomeHandle, this, &ACreteCPPGameMode::DetermineMatchOutcome, 1.0f, false, (float)RemainingGameTime);
			GetWorldTimerManager().SetTimer(CountDownUntilGameOverHandle, this, &ACreteCPPGameMode::CountDownUntilGameOver, 1.0f, true, 0.0f);
		}
	}
}

void ACreteCPPGameMode::HandleProcessTermination()
{
	if (ProcessTerminateState.Status)
	{
		GetWorldTimerManager().ClearTimer(CountDownUntilGameOverHandle);
		GetWorldTimerManager().ClearTimer(HandleProcessTerminationHandle);
		GetWorldTimerManager().ClearTimer(HandleGameSessionUpdateHandle);

		FString ProcessInteruptionMessage;

		if (ProcessTerminateState.TerminationTime <= 0L)
		{
			ProcessInteruptionMessage = "Server process could shut down at any time.";
		}
		else
		{
			long TimeLeft = (long)(ProcessTerminateState.TerminationTime - FDateTime::Now().ToUnixTimestamp());
			ProcessInteruptionMessage = FString::Printf(TEXT("Server process scheduled to terminate in %ld seconds"), TimeLeft);
		}

		if (GameState != nullptr)
		{
			ACreteGameState* CreteGameState = Cast<ACreteGameState>(GameState);
			if (CreteGameState != nullptr)
			{
				CreteGameState->LatestEvent = ProcessInteruptionMessage;
			}
		}

		GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 10.0f);
	}
}

void ACreteCPPGameMode::OnRecordMatchResultResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	GetWorldTimerManager().SetTimer(EndGameHandle, this, &ACreteCPPGameMode::EndGame, 1.0f, false, 5.0f);
}
