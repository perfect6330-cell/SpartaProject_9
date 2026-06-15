#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "ChatXGameModeBase.generated.h"

class AChatXPlayerController;
class AChatXPlayerState;

UCLASS()
class CHATX_API AChatXGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChatXGameModeBase();

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void SubmitPlayerInput(AChatXPlayerController* PlayerController, const FString& RawInput);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule")
	int32 AnswerLength;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule")
	int32 MinDigit;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule")
	int32 MaxDigit;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule")
	int32 MaxAttemptCount;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule", meta = (ClampMin = "1"))
	int32 MinPlayersToStart;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|UI", meta = (ClampMin = "1.0"))
	float RuleGuideDisplayTime;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|UI", meta = (ClampMin = "1.0"))
	float TurnGuideDisplayTime;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Turn")
	bool bUseTurnSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Turn", meta = (EditCondition = "bUseTurnSystem", ClampMin = "1.0"))
	float TurnTimeLimit;

	UPROPERTY(EditDefaultsOnly, Category = "Number Baseball|Rule", meta = (ClampMin = "0.1"))
	float RestartDelay;

private:
	FString SecretAnswer;

	UPROPERTY()
	TArray<TObjectPtr<AChatXPlayerState>> TurnOrder;

	int32 CurrentTurnIndex;
	bool bRoundInProgress;

	FTimerHandle TurnTimerHandle;
	FTimerHandle RestartTimerHandle;

	void GenerateRandomNumbers();
	bool ValidateInput(const FString& Input, FString& OutErrorMessage) const;
	FString CheckAnswer(const FString& Input, int32& OutStrikeCount, int32& OutBallCount) const;

	void ResetGame();
	void EndRound(AChatXPlayerState* WinnerPlayerState);
	bool IsDrawConditionMet() const;
	bool HasEnoughPlayersToStart() const;
	int32 GetReadyPlayerCount() const;
	void TryStartRoundIfPossible();
	FString GetRuleGuideMessage() const;
	FString GetWaitingForPlayersMessage() const;
	void SendRuleGuideToPlayer(AChatXPlayerController* PlayerController) const;
	void SendCurrentTurnGuideToPlayer(AChatXPlayerController* PlayerController) const;

	void BuildTurnOrder(AChatXPlayerState* PreferredCurrentPlayerState = nullptr);
	AChatXPlayerState* GetCurrentTurnPlayerState() const;
	void StartCurrentTurn();
	void AdvanceTurn();
	void HandleTurnTimeout();

	FString GetPlayerDisplayName(const AChatXPlayerState* PlayerState) const;
	void BroadcastSystemMessage(const FString& Message, bool bShowAsAnnouncement = false, float AnnouncementDisplayTime = -1.0f) const;
	void SendPrivateMessage(AChatXPlayerController* PlayerController, const FString& Message) const;
};
