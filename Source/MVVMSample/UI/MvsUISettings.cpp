// Copyright IG. All Rights Reserved.

#include "UI/MvsUISettings.h"

#include "UI/MvsHudWidget.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/ControlsScreen.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"

DEFINE_LOG_CATEGORY(LogMvsUILoading);

UMvsUISettings::UMvsUISettings()
{
	CategoryName = TEXT("Game");
	HudScreenClass = UMvsHudWidget::StaticClass();
	PauseMenuClass = UPauseMenuScreen::StaticClass();
	SettingsScreenClass = USettingsScreen::StaticClass();
	GadgetWheelClass = UGadgetWheelScreen::StaticClass();
	ClueLogClass = UClueLogScreen::StaticClass();
	ControlsScreenClass = UControlsScreen::StaticClass();
	ClueEntryClass = FSoftClassPath(TEXT("/Game/UI/WBP_ClueEntry.WBP_ClueEntry_C"));
	ForensicVisionMaterial = FSoftObjectPath(TEXT("/Game/Materials/M_ForensicVision.M_ForensicVision"));
	ForensicOverlayMaterial = FSoftObjectPath(TEXT("/Game/Materials/M_ForensicOverlay_UI.M_ForensicOverlay_UI"));
}

TConstArrayView<FMvsScreenShortcut> UMvsUISettings::GetShortcuts()
{
	static const FMvsScreenShortcut Shortcuts[] = {
		{ TEXT("Pause"),       EMvsUILayer::Menu,     EMvsShortcutKind::OpenOrBack, &UMvsUISettings::PauseMenuClass },
		{ TEXT("ClueLog"),     EMvsUILayer::Menu,     EMvsShortcutKind::Toggle,     &UMvsUISettings::ClueLogClass },
		{ TEXT("GadgetWheel"), EMvsUILayer::GameMenu, EMvsShortcutKind::Hold,       &UMvsUISettings::GadgetWheelClass },
	};
	return Shortcuts;
}

const FMvsScreenShortcut* UMvsUISettings::FindShortcut(FName Action)
{
	return GetShortcuts().FindByPredicate([Action](const FMvsScreenShortcut& Shortcut) { return Shortcut.Action == Action; });
}

const FMvsScreenShortcut* UMvsUISettings::FindShortcutFor(const UCommonActivatableWidget* Screen) const
{
	if (!Screen)
	{
		return nullptr;
	}
	return GetShortcuts().FindByPredicate([this, Screen](const FMvsScreenShortcut& Shortcut)
	{
		const UClass* Class = (this->*Shortcut.Screen).Get();
		return Class && Screen->IsA(Class);
	});
}

TSubclassOf<UCommonActivatableWidget> UMvsUISettings::ResolveScreen(const FMvsScreenShortcut& Shortcut) const
{
	return Resolve(this->*Shortcut.Screen);
}

TArray<FSoftObjectPath> UMvsUISettings::GetPreloadPaths() const
{
	TArray<FSoftObjectPath> Paths;
	for (const TSoftClassPtr<UCommonActivatableWidget>* Screen : { &PauseMenuClass, &SettingsScreenClass, &GadgetWheelClass, &ClueLogClass, &ControlsScreenClass })
	{
		if (!Screen->IsNull())
		{
			Paths.Add(Screen->ToSoftObjectPath());
		}
	}
	if (!ClueEntryClass.IsNull())
	{
		Paths.Add(ClueEntryClass.ToSoftObjectPath());
	}
	return Paths;
}
