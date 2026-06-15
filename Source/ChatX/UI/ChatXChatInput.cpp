#include "UI/ChatXChatInput.h"
#include "Components/EditableTextBox.h"
#include "Player/ChatXPlayerController.h"

void UChatXChatInput::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(EditableTextBox_ChatInput) == false)
	{
		return;
	}

	EditableTextBox_ChatInput->SetHintText(FText::FromString(TEXT("숫자 3개 입력 후 Enter 예: 386")));
	EditableTextBox_ChatInput->SetKeyboardFocus();

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == false)
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UChatXChatInput::NativeDestruct()
{
	Super::NativeDestruct();

	if (IsValid(EditableTextBox_ChatInput) == false)
	{
		return;
	}

	if (EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &ThisClass::OnChatInputTextCommitted) == true)
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::OnChatInputTextCommitted);
	}
}

void UChatXChatInput::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	APlayerController* OwningPlayerController = GetOwningPlayer();
	if (IsValid(OwningPlayerController) == false)
	{
		return;
	}

	AChatXPlayerController* OwningChatXPlayerController = Cast<AChatXPlayerController>(OwningPlayerController);
	if (IsValid(OwningChatXPlayerController) == false)
	{
		return;
	}

	OwningChatXPlayerController->SetChatMessageString(Text.ToString());
	EditableTextBox_ChatInput->SetText(FText());
}
