// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Containers/Ticker.h"
#include "UObject/Object.h"
#include "ViewModels/MvsSubscriptions.h"
#include "MvsViewModelBinders.generated.h"

class AMvsCharacter;
class UClueDataAsset;
class UClueListViewModel;
class UComboViewModel;
class UForensicViewModel;
class UEnhancedInputUserSettings;
class UGadgetBarViewModel;
class UMvsSettingsSubsystem;
class ULocalPlayer;
class UMVVMViewModelBase;
class UObjectivesViewModel;
class UPlayerVitalsViewModel;
class USubtitleViewModel;
class UThreatViewModel;

/**
 * Wires one gameplay feature into its view models: creates the view models, subscribes to the feature's components when
 * a character is bound and pushes their current state. Binders never see widgets, and gameplay never sees binders.
 *
 * Subscriptions go into FMvsSubscriptions, so unbinding is one Reset and no binder tracks delegate handles.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UMvsViewModelBinder : public UObject
{
	GENERATED_BODY()

public:
	/** Creates the view models. Called once, when the local player's view-model subsystem starts. */
	virtual void Initialize(ULocalPlayer& Player) {}
	/** Subscribes to Character's components and pushes their state. The previous character is unbound first. */
	virtual void Bind(AMvsCharacter& Character) {}
	/** Drops every subscription to the character. */
	virtual void Unbind() { Subscriptions.Reset(); }
	/** Called before the binder goes away. */
	virtual void Deinitialize() { Unbind(); }

	const TArray<TObjectPtr<UMVVMViewModelBase>>& GetViewModels() const { return ViewModels; }

protected:
	template<typename TViewModel>
	TViewModel* AddViewModel()
	{
		TViewModel* ViewModel = NewObject<TViewModel>(this);
		ViewModels.Add(ViewModel);
		return ViewModel;
	}

	FMvsSubscriptions Subscriptions;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMVVMViewModelBase>> ViewModels;
};

/** Health into the vitals view model. */
UCLASS()
class UMvsVitalsBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> Vitals;
};

/**
 * The gadget bar: slot definitions, cooldowns, the last-used gadget, key hints that follow rebinding, and the bar's
 * "use" command back into gameplay.
 */
UCLASS()
class UMvsGadgetBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;
	virtual void Unbind() override;

private:
	/** Key hints from the player's current bindings (rebinding included). */
	void RefreshHotkeys();
	/** Enhanced Input's settings-changed event is dynamic, so it cannot go through FMvsSubscriptions. */
	UFUNCTION()
	void HandleInputSettingsChanged(UEnhancedInputUserSettings* InputSettings) { RefreshHotkeys(); }

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBar;

	TWeakObjectPtr<ULocalPlayer> Player;
	TWeakObjectPtr<UEnhancedInputUserSettings> BoundInputSettings;
};

/** The combo meter. */
UCLASS()
class UMvsComboBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> Combo;
};

/** Forensic mode: on / off, its fade, and clue analysis progress. */
UCLASS()
class UMvsForensicBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UForensicViewModel> Forensic;
};

/** Hostiles: they live in the world, not on the character, so this listens to the world's threat subsystem. */
UCLASS()
class UMvsThreatBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UThreatViewModel> Threats;
};

/**
 * The investigation: the case file (one entry per clue in the level), the objective it drives, and the subtitle line
 * read out when a clue is found (whose size and backing follow accessibility settings).
 */
UCLASS()
class UMvsClueBinder : public UMvsViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AMvsCharacter& Character) override;
	virtual void Deinitialize() override;

#if !UE_BUILD_SHIPPING
	/** Dev aid: appends fake undiscovered clues (flagged IsDebug); they never count toward the objective. */
	void AddDebugClues(int32 Count);
#endif

private:
	void Rebuild(const AMvsCharacter* Character);
	void HandleClueScanned(const UClueDataAsset* Clue);
	void RefreshObjectives();
	void ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds);

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> Objectives;

	UPROPERTY(Transient)
	TObjectPtr<USubtitleViewModel> Subtitles;

	FMvsSettingsListener SettingsListener;
	FTSTicker::FDelegateHandle SubtitleHideHandle;
	/** The character's world, for whether the game is paused while a subtitle counts down. */
	TWeakObjectPtr<UWorld> World;
};
