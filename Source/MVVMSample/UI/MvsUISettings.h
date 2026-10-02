// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UI/Layout/MvsUITypes.h"
#include "MvsUISettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMvsUILoading, Log, All);

class UCommonActivatableWidget;
class UMaterialInterface;
class UUserWidget;
class UMvsUISettings;

/** What a screen shortcut's key does (see FMvsScreenShortcut). */
enum class EMvsShortcutKind : uint8
{
	/** Opens the screen when nothing is open; otherwise closes the topmost screen (pause). */
	OpenOrBack,
	/** Opens the screen on top of whatever menu is open, and closes it when it is the top menu (the case file).
	 *  Every open screen answers the key, since gameplay input is blocked while a menu is up. */
	Toggle,
	/** Opens the screen from gameplay only (no menu up, its layer free); the screen closes itself when the key is
	 *  released (the gadget wheel). */
	Hold,
};

/** A gameplay action that opens a screen. The UI subsystem, the player controller and the base screen read these. */
struct FMvsScreenShortcut
{
	FName Action;
	EMvsUILayer Layer;
	EMvsShortcutKind Kind;
	/** Which of the settings' screen classes it opens, so a class swapped in config (a designer's WBP_) is the one used. */
	TSoftClassPtr<UCommonActivatableWidget> UMvsUISettings::* Screen;
};

/** Project UI configuration (Project Settings > Game > Mvs UI). Lets content swap the HUD without code. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mvs UI"))
class MVVMSAMPLE_API UMvsUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMvsUISettings();

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> HudScreenClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> PauseMenuClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> SettingsScreenClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> GadgetWheelClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> ClueLogClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> ControlsScreenClass;

	/**
	 * Row widget for the clue log. The list view requires a Blueprint subclass of UClueEntryWidget in the editor,
	 * which also gives designers an asset to restyle. Falls back to the native class (fine in cooked builds).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UUserWidget> ClueEntryClass;

	/** Post-process material for Forensic Mode (domain: Post Process). Created by Scripts/CreateForensicAssets.py. */
	UPROPERTY(Config, EditAnywhere, Category = "Forensic")
	TSoftObjectPtr<UMaterialInterface> ForensicVisionMaterial;

	/** Full-screen UI material for the Forensic Mode overlay (domain: User Interface). */
	UPROPERTY(Config, EditAnywhere, Category = "Forensic")
	TSoftObjectPtr<UMaterialInterface> ForensicOverlayMaterial;

	/** The gameplay actions that open screens (second review 9): the generic UI layer reads them instead of knowing features. */
	static TConstArrayView<FMvsScreenShortcut> GetShortcuts();
	static const FMvsScreenShortcut* FindShortcut(FName Action);
	/** The shortcut whose screen Screen is (or derives from), if any. */
	const FMvsScreenShortcut* FindShortcutFor(const UCommonActivatableWidget* Screen) const;
	TSubclassOf<UCommonActivatableWidget> ResolveScreen(const FMvsScreenShortcut& Shortcut) const;

	/** Every class a screen or a key press may need: the screens opened on input, and the case file's entry class. */
	TArray<FSoftObjectPath> GetPreloadPaths() const;

	/**
	 * The loaded class. Screen classes are preloaded when the UI layout is created (UMvsUISubsystem), so this
	 * normally only reads; a class that is not loaded yet is loaded on the spot, with a warning, since that is a hitch.
	 */
	template<typename TClass>
	static TSubclassOf<TClass> Resolve(const TSoftClassPtr<TClass>& Class)
	{
		if (Class.IsNull() || Class.Get())
		{
			return Class.Get();
		}
		UE_LOG(LogMvsUILoading, Warning, TEXT("%s was not preloaded; loading it synchronously."), *Class.ToString());
		return Class.LoadSynchronous();
	}
};
