#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "ChatXPlayerController.generated.h"

class UChatXAnnouncementWidget;
class UChatXChatInput;

UCLASS()
class CHATX_API AChatXPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	void SetChatMessageString(const FString& InputChatMessageString);
	void PrintChatMessageString(const FString& InChatMessageString);

	UFUNCTION(Server, Reliable)
	void ServerSubmitChatMessage(const FString& InChatMessageString);

	UFUNCTION(Client, Reliable)
	void ClientReceiveSystemMessage(const FString& InMessageString);

	UFUNCTION(Client, Reliable)
	void ClientShowAnnouncementMessage(const FString& InMessageString, float DisplayTime);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UChatXChatInput> ChatInputWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UChatXAnnouncementWidget> AnnouncementWidgetClass;

	UPROPERTY()
	TObjectPtr<UChatXChatInput> ChatInputWidgetInstance;

	UPROPERTY()
	TObjectPtr<UChatXAnnouncementWidget> AnnouncementWidgetInstance;

private:
	FTimerHandle AnnouncementHideTimerHandle;
	FString ChatMessageString;

	void HideAnnouncementWidget();
};
