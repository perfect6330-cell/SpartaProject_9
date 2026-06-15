#include "Player/ChatXPlayerState.h"
#include "Net/UnrealNetwork.h"

AChatXPlayerState::AChatXPlayerState()
{
	CurrentAttemptCount = 0;
	MaxAttemptCount = 3;
}

void AChatXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AChatXPlayerState, CurrentAttemptCount);
	DOREPLIFETIME(AChatXPlayerState, MaxAttemptCount);
}

int32 AChatXPlayerState::GetRemainingAttemptCount() const
{
	return FMath::Max(0, MaxAttemptCount - CurrentAttemptCount);
}

bool AChatXPlayerState::CanTryAnswer() const
{
	return CurrentAttemptCount < MaxAttemptCount;
}

FString AChatXPlayerState::GetAttemptProgressText() const
{
	return FString::Printf(TEXT("[%d / %d]"), CurrentAttemptCount, MaxAttemptCount);
}

void AChatXPlayerState::ResetAttemptCount()
{
	CurrentAttemptCount = 0;
}

void AChatXPlayerState::IncreaseAttemptCount()
{
	if (CanTryAnswer() == true)
	{
		++CurrentAttemptCount;
	}
}

void AChatXPlayerState::SetMaxAttemptCount(int32 InMaxAttemptCount)
{
	MaxAttemptCount = FMath::Max(1, InMaxAttemptCount);
	CurrentAttemptCount = FMath::Clamp(CurrentAttemptCount, 0, MaxAttemptCount);
}
