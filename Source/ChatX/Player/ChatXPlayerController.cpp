#include "Player/ChatXPlayerController.h"
#include "Game/ChatXGameModeBase.h"
#include "UI/ChatXAnnouncementWidget.h"
#include "UI/ChatXChatInput.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"

void AChatXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	FInputModeGameAndUI InputModeGameAndUI;
	InputModeGameAndUI.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputModeGameAndUI);
	bShowMouseCursor = true;

	if (IsValid(ChatInputWidgetClass) == true)
	{
		ChatInputWidgetInstance = CreateWidget<UChatXChatInput>(this, ChatInputWidgetClass);
		if (IsValid(ChatInputWidgetInstance) == true)
		{
			ChatInputWidgetInstance->AddToViewport();
		}
	}

	if (IsValid(AnnouncementWidgetClass) == true)
	{
		AnnouncementWidgetInstance = CreateWidget<UChatXAnnouncementWidget>(this, AnnouncementWidgetClass);
		if (IsValid(AnnouncementWidgetInstance) == true)
		{
			AnnouncementWidgetInstance->AddToViewport(10);
			AnnouncementWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void AChatXPlayerController::SetChatMessageString(const FString& InputChatMessageString)
{
	ChatMessageString = InputChatMessageString.TrimStartAndEnd();

	if (ChatMessageString.IsEmpty() == true)
	{
		return;
	}

	ServerSubmitChatMessage(ChatMessageString);
}

void AChatXPlayerController::PrintChatMessageString(const FString& InChatMessageString)
{
	UKismetSystemLibrary::PrintString(this, InChatMessageString, true, true, FLinearColor::Red, 5.0f);
}

void AChatXPlayerController::ServerSubmitChatMessage_Implementation(const FString& InChatMessageString)
{
	AChatXGameModeBase* ChatXGameMode = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<AChatXGameModeBase>() : nullptr;
	if (IsValid(ChatXGameMode) == true)
	{
		ChatXGameMode->SubmitPlayerInput(this, InChatMessageString);
	}
}

void AChatXPlayerController::ClientReceiveSystemMessage_Implementation(const FString& InMessageString)
{
	PrintChatMessageString(InMessageString);
}

void AChatXPlayerController::ClientShowAnnouncementMessage_Implementation(const FString& InMessageString, float DisplayTime)
{
	if (IsValid(AnnouncementWidgetInstance) == false)
	{
		PrintChatMessageString(InMessageString);
		return;
	}

	AnnouncementWidgetInstance->SetAnnouncementMessage(InMessageString);
	AnnouncementWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	GetWorldTimerManager().ClearTimer(AnnouncementHideTimerHandle);
	GetWorldTimerManager().SetTimer(AnnouncementHideTimerHandle, this, &ThisClass::HideAnnouncementWidget, DisplayTime, false);
}

void AChatXPlayerController::HideAnnouncementWidget()
{
	if (IsValid(AnnouncementWidgetInstance) == true)
	{
		AnnouncementWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}
