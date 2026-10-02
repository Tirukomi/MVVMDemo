// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MvsSettingsSubsystem.generated.h"

class FConfigFile;

/** Fired whenever the live (previewed or committed) settings change. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMvsSettingsChanged, const FMvsSettingsData&);

/**
 * Owns the player's UI and accessibility settings. Loads them at start-up, applies their side effects
 * (language, UI scale) and tells widgets to restyle. A change is previewed (applied, not saved) until it is committed
 * or reverted. The settings screen edits them through its own USettingsViewModel, which calls these; the subsystem
 * knows no view model (second review 10).
 */
UCLASS()
class MVVMSAMPLE_API UMvsSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Convenience lookup from any UObject that has a world (widgets, components). Null if unavailable. */
	static UMvsSettingsSubsystem* Get(const UObject* WorldContext);

	/** The settings' section in the user's GameUserSettings.ini. */
	static const TCHAR* GetConfigSection();

	/**
	 * Settings saved before the code prefix changed (Gotham to Mvs, review 36) sit under the old class name. Moves them to
	 * the current section once, so existing players keep them; does nothing if the current section already exists.
	 * @return true if anything moved
	 */
	static bool MigrateLegacySettings(FConfigFile& File);

	/** The settings in effect, a previewed change included. */
	const FMvsSettingsData& GetSettings() const { return Live; }
	/** The last committed settings, the ones in the user's config. */
	const FMvsSettingsData& GetSaved() const { return Saved; }
	bool HasUnsavedChanges() const { return Live != Saved; }

	/** Applies Data (language, UI scale, restyle) without saving it. */
	void Preview(const FMvsSettingsData& Data);
	/** Applies Data and saves it. */
	void Commit(const FMvsSettingsData& Data);
	/** Back to the saved settings. */
	void Revert() { Preview(Saved); }

	FLinearColor GetColor(EMvsColorToken Token) const { return MvsPalette::Resolve(Token, Live.ColorMode, Live.bHighContrast); }
	float GetPanelAlpha() const { return MvsPalette::PanelAlpha(Live.bHighContrast); }

	FOnMvsSettingsChanged OnSettingsChanged;

private:
	void ApplyEffects(const FMvsSettingsData& Data, bool bLanguageChanged);

	FMvsSettingsData Live;
	FMvsSettingsData Saved;

	/**
	 * The culture before the game applied the player's language. The culture is process-wide (in the editor it is the
	 * editor's too), so it is put back when the game instance shuts down, e.g. at the end of a play-in-editor session.
	 */
	FString CultureBeforeGame;
};
