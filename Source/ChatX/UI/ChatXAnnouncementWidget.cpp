#include "UI/ChatXAnnouncementWidget.h"
#include "Components/TextBlock.h"

void UChatXAnnouncementWidget::SetAnnouncementMessage(const FString& Message)
{
	if (IsValid(TextBlock_Announcement) == true)
	{
		TextBlock_Announcement->SetText(FText::FromString(Message));
	}
}
