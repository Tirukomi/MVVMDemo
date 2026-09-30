// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "UI/Screens/GothamScreen.h"
#include "SettingsScreen.generated.h"

class UGothamMenuList;
class UGothamOptionRow;
class UGothamPanel;
class UGothamSwitcher;
class UGothamTabList;
class USettingsViewModel;
class UTextBlock;

/**
 * Language and accessibility options in Common UI tabs (Display, Accessibility, Controls, Language; Q / E or the
 * shoulder buttons switch). Each page is a highlight list of option rows, and a detail pane explains the focused
 * option. Changes preview live; Apply keeps them, and leaving the screen any other way (Back, Esc, B) discards
 * unapplied changes. Every control is reachable with a gamepad.
 */
UCLASS()
class MVVMSAMPLE_API USettingsScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnClosed() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void OnPaletteChanged() override;

private:
	void RefreshDirtyNote();
	void OnViewModelChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void OpenControls();
	void OnCurrentItemChanged(UWidget* Item);
	void ShowDetail(UWidget* Item);
	void OnPageShown(UWidget* Page, int32 Index);

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGothamOptionRow>> Rows;
	TArray<EGothamSetting> RowSettings;

	/** One highlight list per tab page, in tab order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UGothamMenuList>> Pages;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> KeyBindingsItem;

	UPROPERTY(Transient)
	TObjectPtr<UGothamTabList> Tabs;

	UPROPERTY(Transient)
	TObjectPtr<UGothamSwitcher> Switcher;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> DetailPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailBody;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DirtyNote;

	TWeakObjectPtr<UWidget> DetailItem;
};
