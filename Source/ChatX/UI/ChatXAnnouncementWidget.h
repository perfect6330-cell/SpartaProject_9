#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatXAnnouncementWidget.generated.h"

class UTextBlock;

UCLASS()
class CHATX_API UChatXAnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Announcement")
	void SetAnnouncementMessage(const FString& Message);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextBlock_Announcement;
};
