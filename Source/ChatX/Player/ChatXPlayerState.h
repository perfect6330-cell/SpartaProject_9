#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ChatXPlayerState.generated.h"

UCLASS()
class CHATX_API AChatXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AChatXPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Number Baseball")
	int32 GetCurrentAttemptCount() const { return CurrentAttemptCount; }

	UFUNCTION(BlueprintPure, Category = "Number Baseball")
	int32 GetMaxAttemptCount() const { return MaxAttemptCount; }

	UFUNCTION(BlueprintPure, Category = "Number Baseball")
	int32 GetRemainingAttemptCount() const;

	UFUNCTION(BlueprintPure, Category = "Number Baseball")
	bool CanTryAnswer() const;

	UFUNCTION(BlueprintPure, Category = "Number Baseball")
	FString GetAttemptProgressText() const;

	void ResetAttemptCount();
	void IncreaseAttemptCount();
	void SetMaxAttemptCount(int32 InMaxAttemptCount);

private:
	UPROPERTY(Replicated)
	int32 CurrentAttemptCount;

	UPROPERTY(Replicated)
	int32 MaxAttemptCount;
};
