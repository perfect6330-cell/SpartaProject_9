#include "State/ChatXGameStateBase.h"
#include "Engine/Engine.h"

void AChatXGameStateBase::MulticastBroadcastSystemMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, Message);
	}

	OnSystemMessageReceived.Broadcast(Message);
}
