// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Blueprint/UserWidget.h"
#include "GothamOptionRow.generated.h"

class UGothamButton;
class USettingsViewModel;
class UTextBlock;

/**
 * One settings row: label, a "<" button, the current value, a ">" button. Both buttons are focusable, so
 * gamepad and keyboard navigation reach every option with the standard directional keys.
 */
UCLASS()
class MVVMSAMPLE_API UGothamOptionRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(EGothamSetting InSetting, USettingsViewModel* InViewModel);

	/** The widget that should receive focus when the row is first targeted. */
	UWidget* GetPrimaryFocusTarget() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	EGothamSetting Setting = EGothamSetting::Language;

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<UGothamButton> NextButton;

	UPROPERTY(Transient)
	TObjectPtr<UGothamButton> PrevButton;
};
