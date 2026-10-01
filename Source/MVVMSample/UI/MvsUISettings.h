// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MvsUISettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMvsUILoading, Log, All);

class UCommonActivatableWidget;
class UMaterialInterface;
class UUserWidget;

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
