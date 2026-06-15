#include "Game/ChatXGameModeBase.h"
#include "Player/ChatXPlayerController.h"
#include "Player/ChatXPlayerState.h"
#include "State/ChatXGameStateBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "Engine/World.h"

AChatXGameModeBase::AChatXGameModeBase()
{
	PlayerControllerClass = AChatXPlayerController::StaticClass();
	PlayerStateClass = AChatXPlayerState::StaticClass();
	GameStateClass = AChatXGameStateBase::StaticClass();

	AnswerLength = 3;
	MinDigit = 1;
	MaxDigit = 9;
	MaxAttemptCount = 3;
	MinPlayersToStart = 2;
	RuleGuideDisplayTime = 10.0f;
	TurnGuideDisplayTime = 5.0f;
	bUseTurnSystem = true;
	TurnTimeLimit = 15.0f;
	RestartDelay = 3.0f;
	CurrentTurnIndex = INDEX_NONE;
	bRoundInProgress = false;
}

void AChatXGameModeBase::StartPlay()
{
	Super::StartPlay();

	bRoundInProgress = false;
	CurrentTurnIndex = INDEX_NONE;
}

void AChatXGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AChatXPlayerState* NewChatXPlayerState = NewPlayer != nullptr ? NewPlayer->GetPlayerState<AChatXPlayerState>() : nullptr;
	AChatXPlayerController* NewChatXPlayerController = Cast<AChatXPlayerController>(NewPlayer);

	if (IsValid(NewChatXPlayerState) == true)
	{
		NewChatXPlayerState->SetMaxAttemptCount(MaxAttemptCount);
		NewChatXPlayerState->ResetAttemptCount();
	}

	AChatXPlayerState* PreviousCurrentTurnPlayerState = GetCurrentTurnPlayerState();
	BuildTurnOrder(PreviousCurrentTurnPlayerState);

	if (IsValid(NewChatXPlayerState) == true)
	{
		BroadcastSystemMessage(FString::Printf(TEXT("[입장] %s 님이 참가했습니다. 현재 인원 %d/%d"),
			*GetPlayerDisplayName(NewChatXPlayerState),
			GetReadyPlayerCount(),
			MinPlayersToStart));
	}

	SendRuleGuideToPlayer(NewChatXPlayerController);

	if (bRoundInProgress == false)
	{
		TryStartRoundIfPossible();
		return;
	}

	SendCurrentTurnGuideToPlayer(NewChatXPlayerController);

	if (bUseTurnSystem == true && GetWorldTimerManager().IsTimerActive(TurnTimerHandle) == false)
	{
		StartCurrentTurn();
	}
}

void AChatXGameModeBase::Logout(AController* Exiting)
{
	AChatXPlayerState* ExitingPlayerState = Exiting != nullptr ? Exiting->GetPlayerState<AChatXPlayerState>() : nullptr;
	AChatXPlayerState* PreviousCurrentTurnPlayerState = GetCurrentTurnPlayerState();

	Super::Logout(Exiting);

	const bool bCurrentTurnPlayerExited = PreviousCurrentTurnPlayerState == ExitingPlayerState;
	BuildTurnOrder(bCurrentTurnPlayerExited == true ? nullptr : PreviousCurrentTurnPlayerState);

	if (HasEnoughPlayersToStart() == false)
	{
		bRoundInProgress = false;
		CurrentTurnIndex = INDEX_NONE;
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
		GetWorldTimerManager().ClearTimer(RestartTimerHandle);
		BroadcastSystemMessage(GetWaitingForPlayersMessage(), true, RuleGuideDisplayTime);
		return;
	}

	if (bRoundInProgress == true && bUseTurnSystem == true)
	{
		if (bCurrentTurnPlayerExited == true)
		{
			GetWorldTimerManager().ClearTimer(TurnTimerHandle);
			StartCurrentTurn();
		}
		else if (GetCurrentTurnPlayerState() == nullptr || GetCurrentTurnPlayerState()->CanTryAnswer() == false)
		{
			AdvanceTurn();
		}
	}
}

void AChatXGameModeBase::SubmitPlayerInput(AChatXPlayerController* PlayerController, const FString& RawInput)
{
	if (IsValid(PlayerController) == false)
	{
		return;
	}

	if (HasEnoughPlayersToStart() == false)
	{
		SendPrivateMessage(PlayerController, GetWaitingForPlayersMessage());
		SendRuleGuideToPlayer(PlayerController);
		return;
	}

	if (bRoundInProgress == false)
	{
		TryStartRoundIfPossible();
		if (bRoundInProgress == false)
		{
			SendPrivateMessage(PlayerController, TEXT("[안내] 라운드가 아직 시작되지 않았습니다."));
		}
		return;
	}

	AChatXPlayerState* ChatXPlayerState = PlayerController->GetPlayerState<AChatXPlayerState>();
	if (IsValid(ChatXPlayerState) == false)
	{
		SendPrivateMessage(PlayerController, TEXT("[오류] PlayerState를 찾을 수 없습니다."));
		return;
	}

	if (bUseTurnSystem == true)
	{
		AChatXPlayerState* CurrentTurnPlayerState = GetCurrentTurnPlayerState();
		if (CurrentTurnPlayerState != ChatXPlayerState)
		{
			SendPrivateMessage(PlayerController, FString::Printf(TEXT("[턴 안내] 아직 차례가 아닙니다. 현재 차례: %s"), *GetPlayerDisplayName(CurrentTurnPlayerState)));
			return;
		}
	}

	if (ChatXPlayerState->CanTryAnswer() == false)
	{
		SendPrivateMessage(PlayerController, TEXT("[안내] 이미 3번의 기회를 모두 사용했습니다."));
		return;
	}

	const FString TrimmedInput = RawInput.TrimStartAndEnd();

	FString ErrorMessage;
	if (ValidateInput(TrimmedInput, ErrorMessage) == false)
	{
		SendPrivateMessage(PlayerController, ErrorMessage);
		return;
	}

	ChatXPlayerState->IncreaseAttemptCount();

	int32 StrikeCount = 0;
	int32 BallCount = 0;
	const FString ResultText = CheckAnswer(TrimmedInput, StrikeCount, BallCount);
	const FString AttemptText = ChatXPlayerState->GetAttemptProgressText();
	const int32 RemainingAttemptCount = ChatXPlayerState->GetRemainingAttemptCount();

	BroadcastSystemMessage(FString::Printf(TEXT("%s 입력: %s -> %s  시도 %s / 남은 기회 %d"),
		*GetPlayerDisplayName(ChatXPlayerState),
		*TrimmedInput,
		*ResultText,
		*AttemptText,
		RemainingAttemptCount));

	if (StrikeCount == AnswerLength)
	{
		EndRound(ChatXPlayerState);
		return;
	}

	if (IsDrawConditionMet() == true)
	{
		EndRound(nullptr);
		return;
	}

	if (bUseTurnSystem == true)
	{
		AdvanceTurn();
	}
}

void AChatXGameModeBase::GenerateRandomNumbers()
{
	SecretAnswer.Empty();

	TArray<int32> CandidateDigits;
	for (int32 Digit = MinDigit; Digit <= MaxDigit; ++Digit)
	{
		CandidateDigits.Add(Digit);
	}

	while (SecretAnswer.Len() < AnswerLength && CandidateDigits.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateDigits.Num() - 1);
		SecretAnswer += FString::FromInt(CandidateDigits[RandomIndex]);
		CandidateDigits.RemoveAtSwap(RandomIndex);
	}

	UE_LOG(LogTemp, Warning, TEXT("[Number Baseball] Server Answer: %s"), *SecretAnswer);
}

bool AChatXGameModeBase::ValidateInput(const FString& Input, FString& OutErrorMessage) const
{
	if (Input.Len() != AnswerLength)
	{
		OutErrorMessage = FString::Printf(TEXT("[다시 입력] %d자리 숫자를 입력해주세요. 예: 386"), AnswerLength);
		return false;
	}

	TSet<TCHAR> UsedDigits;
	for (int32 Index = 0; Index < Input.Len(); ++Index)
	{
		const TCHAR Character = Input[Index];
		if (FChar::IsDigit(Character) == false)
		{
			OutErrorMessage = TEXT("[다시 입력] 숫자가 아닌 문자가 포함되어 있습니다. 기회는 소진되지 않습니다.");
			return false;
		}

		const int32 Digit = Character - TEXT('0');
		if (Digit < MinDigit || Digit > MaxDigit)
		{
			OutErrorMessage = FString::Printf(TEXT("[다시 입력] %d~%d 사이의 숫자만 사용할 수 있습니다."), MinDigit, MaxDigit);
			return false;
		}

		if (UsedDigits.Contains(Character) == true)
		{
			OutErrorMessage = TEXT("[다시 입력] 중복되지 않은 숫자를 입력해주세요. 기회는 소진되지 않습니다.");
			return false;
		}

		UsedDigits.Add(Character);
	}

	return true;
}

FString AChatXGameModeBase::CheckAnswer(const FString& Input, int32& OutStrikeCount, int32& OutBallCount) const
{
	OutStrikeCount = 0;
	OutBallCount = 0;

	for (int32 InputIndex = 0; InputIndex < Input.Len(); ++InputIndex)
	{
		if (SecretAnswer.IsValidIndex(InputIndex) == true && Input[InputIndex] == SecretAnswer[InputIndex])
		{
			++OutStrikeCount;
			continue;
		}

		int32 FoundIndex = INDEX_NONE;
		if (SecretAnswer.FindChar(Input[InputIndex], FoundIndex) == true)
		{
			++OutBallCount;
		}
	}

	if (OutStrikeCount == 0 && OutBallCount == 0)
	{
		return TEXT("OUT");
	}

	return FString::Printf(TEXT("%dS%dB"), OutStrikeCount, OutBallCount);
}

void AChatXGameModeBase::ResetGame()
{
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().ClearTimer(RestartTimerHandle);

	BuildTurnOrder();

	if (HasEnoughPlayersToStart() == false)
	{
		bRoundInProgress = false;
		CurrentTurnIndex = INDEX_NONE;
		BroadcastSystemMessage(GetWaitingForPlayersMessage(), true, RuleGuideDisplayTime);
		return;
	}

	GenerateRandomNumbers();

	if (GameState != nullptr)
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			AChatXPlayerState* ChatXPlayerState = Cast<AChatXPlayerState>(PlayerState);
			if (IsValid(ChatXPlayerState) == true)
			{
				ChatXPlayerState->SetMaxAttemptCount(MaxAttemptCount);
				ChatXPlayerState->ResetAttemptCount();
			}
		}
	}

	bRoundInProgress = true;
	BuildTurnOrder();

	BroadcastSystemMessage(GetRuleGuideMessage(), true, RuleGuideDisplayTime);

	if (bUseTurnSystem == true)
	{
		StartCurrentTurn();
	}
}

void AChatXGameModeBase::EndRound(AChatXPlayerState* WinnerPlayerState)
{
	if (bRoundInProgress == false)
	{
		return;
	}

	bRoundInProgress = false;
	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	if (IsValid(WinnerPlayerState) == true)
	{
		BroadcastSystemMessage(FString::Printf(TEXT("[결과] %s 승리! 정답은 %s 입니다. 잠시 후 게임을 리셋합니다."),
			*GetPlayerDisplayName(WinnerPlayerState),
			*SecretAnswer), true, RestartDelay);
	}
	else
	{
		BroadcastSystemMessage(FString::Printf(TEXT("[결과] 무승부! 정답은 %s 입니다. 잠시 후 게임을 리셋합니다."), *SecretAnswer), true, RestartDelay);
	}

	GetWorldTimerManager().SetTimer(RestartTimerHandle, this, &ThisClass::ResetGame, RestartDelay, false);
}

bool AChatXGameModeBase::IsDrawConditionMet() const
{
	if (GameState == nullptr || GameState->PlayerArray.Num() == 0)
	{
		return false;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		const AChatXPlayerState* ChatXPlayerState = Cast<AChatXPlayerState>(PlayerState);
		if (IsValid(ChatXPlayerState) == true && ChatXPlayerState->CanTryAnswer() == true)
		{
			return false;
		}
	}

	return true;
}

bool AChatXGameModeBase::HasEnoughPlayersToStart() const
{
	return GetReadyPlayerCount() >= MinPlayersToStart;
}

int32 AChatXGameModeBase::GetReadyPlayerCount() const
{
	int32 ReadyPlayerCount = 0;

	if (GameState == nullptr)
	{
		return ReadyPlayerCount;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (IsValid(Cast<AChatXPlayerState>(PlayerState)) == true)
		{
			++ReadyPlayerCount;
		}
	}

	return ReadyPlayerCount;
}

void AChatXGameModeBase::TryStartRoundIfPossible()
{
	if (bRoundInProgress == true)
	{
		return;
	}

	if (HasEnoughPlayersToStart() == false)
	{
		BroadcastSystemMessage(GetWaitingForPlayersMessage(), true, RuleGuideDisplayTime);
		return;
	}

	ResetGame();
}

FString AChatXGameModeBase::GetRuleGuideMessage() const
{
	return FString::Printf(
		TEXT("[숫자 야구 규칙]\n")
		TEXT("목표: 서버가 숨긴 %d자리 숫자를 먼저 맞히면 승리합니다.\n")
		TEXT("사용 숫자: %d~%d, 중복 숫자 금지. 예: 386\n")
		TEXT("입력 방법: 아래 채팅창에 386처럼 입력하고 Enter를 누르세요.\n")
		TEXT("판정: S=숫자+자리 모두 맞음 / B=숫자는 맞지만 자리 틀림 / OUT=맞는 숫자 없음.\n")
		TEXT("기회: 플레이어당 %d번. 잘못된 입력은 기회가 줄지 않습니다.\n")
		TEXT("턴: 자기 차례에 %.0f초 안에 입력해야 하며, 시간 초과 시 기회 1회가 소진됩니다."),
		AnswerLength,
		MinDigit,
		MaxDigit,
		MaxAttemptCount,
		TurnTimeLimit);
}

FString AChatXGameModeBase::GetWaitingForPlayersMessage() const
{
	return FString::Printf(TEXT("[대기] 숫자 야구는 최소 %d명이 필요합니다. 현재 인원 %d/%d"),
		MinPlayersToStart,
		GetReadyPlayerCount(),
		MinPlayersToStart);
}

void AChatXGameModeBase::SendRuleGuideToPlayer(AChatXPlayerController* PlayerController) const
{
	SendPrivateMessage(PlayerController, GetRuleGuideMessage());
}

void AChatXGameModeBase::SendCurrentTurnGuideToPlayer(AChatXPlayerController* PlayerController) const
{
	if (bUseTurnSystem == false)
	{
		return;
	}

	const AChatXPlayerState* CurrentTurnPlayerState = GetCurrentTurnPlayerState();
	SendPrivateMessage(PlayerController, FString::Printf(TEXT("[현재 턴] %s 차례입니다. 채팅창에 중복 없는 %d자리 숫자를 입력하세요."),
		*GetPlayerDisplayName(CurrentTurnPlayerState),
		AnswerLength));
}

void AChatXGameModeBase::BuildTurnOrder(AChatXPlayerState* PreferredCurrentPlayerState)
{
	TurnOrder.Empty();

	if (GameState != nullptr)
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			AChatXPlayerState* ChatXPlayerState = Cast<AChatXPlayerState>(PlayerState);
			if (IsValid(ChatXPlayerState) == true)
			{
				TurnOrder.Add(ChatXPlayerState);
			}
		}
	}

	TurnOrder.Sort([](const TObjectPtr<AChatXPlayerState>& Left, const TObjectPtr<AChatXPlayerState>& Right)
	{
		const int32 LeftPlayerId = IsValid(Left.Get()) == true ? Left->GetPlayerId() : MAX_int32;
		const int32 RightPlayerId = IsValid(Right.Get()) == true ? Right->GetPlayerId() : MAX_int32;
		return LeftPlayerId < RightPlayerId;
	});

	CurrentTurnIndex = INDEX_NONE;

	if (IsValid(PreferredCurrentPlayerState) == true)
	{
		for (int32 Index = 0; Index < TurnOrder.Num(); ++Index)
		{
			if (TurnOrder[Index].Get() == PreferredCurrentPlayerState)
			{
				CurrentTurnIndex = Index;
				break;
			}
		}
	}

	if (CurrentTurnIndex == INDEX_NONE && TurnOrder.Num() > 0)
	{
		CurrentTurnIndex = 0;
	}
}

AChatXPlayerState* AChatXGameModeBase::GetCurrentTurnPlayerState() const
{
	if (TurnOrder.IsValidIndex(CurrentTurnIndex) == true)
	{
		return TurnOrder[CurrentTurnIndex];
	}

	return nullptr;
}

void AChatXGameModeBase::StartCurrentTurn()
{
	if (bRoundInProgress == false || bUseTurnSystem == false)
	{
		return;
	}

	if (HasEnoughPlayersToStart() == false)
	{
		bRoundInProgress = false;
		GetWorldTimerManager().ClearTimer(TurnTimerHandle);
		BroadcastSystemMessage(GetWaitingForPlayersMessage(), true, RuleGuideDisplayTime);
		return;
	}

	AChatXPlayerState* CurrentTurnPlayerState = GetCurrentTurnPlayerState();
	if (IsValid(CurrentTurnPlayerState) == false)
	{
		BuildTurnOrder();
		CurrentTurnPlayerState = GetCurrentTurnPlayerState();
	}

	if (IsValid(CurrentTurnPlayerState) == false)
	{
		return;
	}

	if (CurrentTurnPlayerState->CanTryAnswer() == false)
	{
		AdvanceTurn();
		return;
	}

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);
	GetWorldTimerManager().SetTimer(TurnTimerHandle, this, &ThisClass::HandleTurnTimeout, TurnTimeLimit, false);

	BroadcastSystemMessage(FString::Printf(TEXT("[턴] %s 차례입니다. %.0f초 안에 채팅창에 숫자 %d개를 입력하세요. 현재 시도 %s"),
		*GetPlayerDisplayName(CurrentTurnPlayerState),
		TurnTimeLimit,
		AnswerLength,
		*CurrentTurnPlayerState->GetAttemptProgressText()), true, TurnGuideDisplayTime);
}

void AChatXGameModeBase::AdvanceTurn()
{
	if (bRoundInProgress == false || bUseTurnSystem == false)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TurnTimerHandle);

	if (IsDrawConditionMet() == true)
	{
		EndRound(nullptr);
		return;
	}

	BuildTurnOrder(GetCurrentTurnPlayerState());

	if (TurnOrder.Num() == 0)
	{
		return;
	}

	const int32 StartIndex = CurrentTurnIndex == INDEX_NONE ? 0 : CurrentTurnIndex;
	for (int32 Offset = 1; Offset <= TurnOrder.Num(); ++Offset)
	{
		const int32 NextIndex = (StartIndex + Offset) % TurnOrder.Num();
		AChatXPlayerState* CandidatePlayerState = TurnOrder[NextIndex];
		if (IsValid(CandidatePlayerState) == true && CandidatePlayerState->CanTryAnswer() == true)
		{
			CurrentTurnIndex = NextIndex;
			StartCurrentTurn();
			return;
		}
	}

	EndRound(nullptr);
}

void AChatXGameModeBase::HandleTurnTimeout()
{
	if (bRoundInProgress == false)
	{
		return;
	}

	AChatXPlayerState* CurrentTurnPlayerState = GetCurrentTurnPlayerState();
	if (IsValid(CurrentTurnPlayerState) == false)
	{
		AdvanceTurn();
		return;
	}

	if (CurrentTurnPlayerState->CanTryAnswer() == true)
	{
		CurrentTurnPlayerState->IncreaseAttemptCount();
		BroadcastSystemMessage(FString::Printf(TEXT("[시간 초과] %s 님의 기회가 1회 소진되었습니다. 현재 시도 %s / 남은 기회 %d"),
			*GetPlayerDisplayName(CurrentTurnPlayerState),
			*CurrentTurnPlayerState->GetAttemptProgressText(),
			CurrentTurnPlayerState->GetRemainingAttemptCount()));
	}

	AdvanceTurn();
}

FString AChatXGameModeBase::GetPlayerDisplayName(const AChatXPlayerState* PlayerState) const
{
	if (IsValid(PlayerState) == false)
	{
		return TEXT("알 수 없는 플레이어");
	}

	const FString PlayerName = PlayerState->GetPlayerName();
	if (PlayerName.IsEmpty() == false)
	{
		return PlayerName;
	}

	return FString::Printf(TEXT("Player%d"), PlayerState->GetPlayerId());
}

void AChatXGameModeBase::BroadcastSystemMessage(const FString& Message, bool bShowAsAnnouncement, float AnnouncementDisplayTime) const
{
	AChatXGameStateBase* ChatXGameState = GetGameState<AChatXGameStateBase>();
	if (IsValid(ChatXGameState) == true)
	{
		ChatXGameState->MulticastBroadcastSystemMessage(Message);
	}

	if (bShowAsAnnouncement == true && GetWorld() != nullptr)
	{
		const float DisplayTime = AnnouncementDisplayTime > 0.0f ? AnnouncementDisplayTime : RestartDelay;
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AChatXPlayerController* ChatXPlayerController = Cast<AChatXPlayerController>(Iterator->Get());
			if (IsValid(ChatXPlayerController) == true)
			{
				ChatXPlayerController->ClientShowAnnouncementMessage(Message, DisplayTime);
			}
		}
	}
}

void AChatXGameModeBase::SendPrivateMessage(AChatXPlayerController* PlayerController, const FString& Message) const
{
	if (IsValid(PlayerController) == true)
	{
		PlayerController->ClientReceiveSystemMessage(Message);
	}
}
