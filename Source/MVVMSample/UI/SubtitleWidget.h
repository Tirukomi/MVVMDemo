// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "SubtitleWidget.generated.h"

class UGothamPanel;
class USubtitleViewModel;
class UTextBlock;

/** Bottom-of-screen subtitle line. Size and backing panel follow accessibility settings via the view model. */
UCLASS()
class MVVMSAMPLE_API USubtitleWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(USubtitleViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void ApplyTheme() override { Refresh(); }

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<USubtitleViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeakerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LineText;
};
