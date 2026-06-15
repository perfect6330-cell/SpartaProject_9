#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ChatXGameStateBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FChatXSystemMessageDelegate, const FString&, Message);

UCLASS()
class CHATX_API AChatXGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Chat")
	FChatXSystemMessageDelegate OnSystemMessageReceived;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBroadcastSystemMessage(const FString& Message);
};
