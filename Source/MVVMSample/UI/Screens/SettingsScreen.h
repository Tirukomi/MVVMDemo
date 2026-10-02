// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Accessibility/MvsSettingsListener.h"
#include "UI/Screens/MvsScreen.h"
#include "SettingsScreen.generated.h"

class UMvsMenuList;
class UMvsOptionRow;
class UMvsPanel;
class UMvsSwitcher;
class UMvsTabList;
class USettingsViewModel;
class UTextBlock;

/**
 * Language and accessibility options in Common UI tabs (Display, Accessibility, Controls, Language; Q / E or the
 * shoulder buttons switch). Each page is a highlight list of option rows, and a detail pane explains the focused
 * option. Changes preview live; Apply keeps them, and leaving the screen any other way (Back, Esc, B) discards
 * unapplied changes. Every control is reachable with a gamepad.
 */
UCLASS()
class MVVMSAMPLE_API USettingsScreen : public UMvsScreen
{
	GENERATED_BODY()

public:
	/** The screen's own view model over the settings subsystem (null before the screen is first constructed). */
	USettingsViewModel* GetViewModel() const { return ViewModel; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnClosed() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual void ApplyTheme(const FMvsTheme& Theme) override;

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
	TArray<TObjectPtr<UMvsOptionRow>> Rows;
	TArray<EMvsSetting> RowSettings;

	/** One highlight list per tab page, in tab order. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMvsMenuList>> Pages;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> KeyBindingsItem;

	UPROPERTY(Transient)
	TObjectPtr<UMvsTabList> Tabs;

	UPROPERTY(Transient)
	TObjectPtr<UMvsSwitcher> Switcher;

	UPROPERTY(Transient)
	TObjectPtr<UMvsPanel> DetailPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailBody;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DirtyNote;

	TWeakObjectPtr<UWidget> DetailItem;

	/** Keeps the view model in step with the subsystem (a change made elsewhere, a language switch). */
	FMvsSettingsListener ModelListener;
};
