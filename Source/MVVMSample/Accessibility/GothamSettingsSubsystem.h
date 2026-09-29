// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GothamSettingsSubsystem.generated.h"

class USettingsViewModel;

/** Fired whenever the live (previewed or committed) settings change. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGothamSettingsChanged, const FGothamSettingsData&);

/**
 * Owns the player's UI and accessibility settings. Loads them at start-up, applies their side effects
 * (language, UI scale) and tells widgets to restyle. The settings screen edits them through USettingsViewModel.
 */
UCLASS()
class MVVMSAMPLE_API UGothamSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Convenience lookup from any UObject that has a world (widgets, components). Null if unavailable. */
	static UGothamSettingsSubsystem* Get(const UObject* WorldContext);

	const FGothamSettingsData& GetSettings() const { return Live; }
	USettingsViewModel* GetViewModel() const { return ViewModel; }

	FLinearColor GetColor(EGothamColorToken Token) const { return GothamPalette::Resolve(Token, Live.ColorMode, Live.bHighContrast); }
	float GetPanelAlpha() const { return GothamPalette::PanelAlpha(Live.bHighContrast); }

	FOnGothamSettingsChanged OnSettingsChanged;

private:
	void HandlePreview(const FGothamSettingsData& Data);
	void HandleCommitted(const FGothamSettingsData& Data);
	void ApplyEffects(const FGothamSettingsData& Data, bool bLanguageChanged);

	FGothamSettingsData Live;

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;
};
