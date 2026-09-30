// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Blueprint/UserWidget.h"
#include "GothamOptionRow.generated.h"

class UGothamSelectorDecor;
class USettingsViewModel;
class UTextBlock;

/**
 * One settings row: the label on the left and a compact value selector (chevrons, value, position pips) on the right.
 * The whole row is one focus stop. Left / right (keys, d-pad or stick) change the value instead of moving focus,
 * Enter / A steps forward, and a click on either half of the selector steps that way. Hover moves focus, so the
 * menu highlight follows the mouse too.
 */
UCLASS()
class MVVMSAMPLE_API UGothamOptionRow : public UUserWidget
{
	GENERATED_BODY()

public:
	UGothamOptionRow(const FObjectInitializer& ObjectInitializer);

	void Setup(EGothamSetting InSetting, USettingsViewModel* InViewModel);
	EGothamSetting GetSetting() const { return Setting; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	void Step(int32 Direction);
	void Refresh();
	void ApplyColors();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	EGothamSetting Setting = EGothamSetting::Language;
	bool bFocused = false;
	FGothamSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<UGothamSelectorDecor> Decor;
};
