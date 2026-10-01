// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Containers/Ticker.h"
#include "UObject/Object.h"
#include "ViewModels/GothamSubscriptions.h"
#include "GothamViewModelBinders.generated.h"

class AGothamCharacter;
class UClueDataAsset;
class UClueListViewModel;
class UComboViewModel;
class UDetectiveViewModel;
class UEnhancedInputUserSettings;
class UGadgetBarViewModel;
class UGothamSettingsSubsystem;
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
 * Subscriptions go into FGothamSubscriptions, so unbinding is one Reset and no binder tracks delegate handles.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UGothamViewModelBinder : public UObject
{
	GENERATED_BODY()

public:
	/** Creates the view models. Called once, when the local player's view-model subsystem starts. */
	virtual void Initialize(ULocalPlayer& Player) {}
	/** Subscribes to Character's components and pushes their state. The previous character is unbound first. */
	virtual void Bind(AGothamCharacter& Character) {}
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

	FGothamSubscriptions Subscriptions;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMVVMViewModelBase>> ViewModels;
};

/** Health into the vitals view model. */
UCLASS()
class UGothamVitalsBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> Vitals;
};

/**
 * The gadget bar: slot definitions, cooldowns, the last-used gadget, key hints that follow rebinding, and the bar's
 * "use" command back into gameplay.
 */
UCLASS()
class UGothamGadgetBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;
	virtual void Unbind() override;

private:
	/** Key hints from the player's current bindings (rebinding included). */
	void RefreshHotkeys();
	/** Enhanced Input's settings-changed event is dynamic, so it cannot go through FGothamSubscriptions. */
	UFUNCTION()
	void HandleInputSettingsChanged(UEnhancedInputUserSettings* InputSettings) { RefreshHotkeys(); }

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBar;

	TWeakObjectPtr<ULocalPlayer> Player;
	TWeakObjectPtr<UEnhancedInputUserSettings> BoundInputSettings;
};

/** The combo meter. */
UCLASS()
class UGothamComboBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> Combo;
};

/** Detective mode: on / off, its fade, and clue analysis progress. */
UCLASS()
class UGothamDetectiveBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> Detective;
};

/** Hostiles: they live in the world, not on the character, so this listens to the world's threat subsystem. */
UCLASS()
class UGothamThreatBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UThreatViewModel> Threats;
};

/**
 * The investigation: the case file (one entry per clue in the level), the objective it drives, and the subtitle line
 * read out when a clue is found (whose size and backing follow accessibility settings).
 */
UCLASS()
class UGothamClueBinder : public UGothamViewModelBinder
{
	GENERATED_BODY()

public:
	virtual void Initialize(ULocalPlayer& Player) override;
	virtual void Bind(AGothamCharacter& Character) override;
	virtual void Deinitialize() override;

	/** Dev aid: appends fake undiscovered clues (flagged IsDebug); they never count toward the objective. */
	void AddDebugClues(int32 Count);

private:
	void Rebuild(const AGothamCharacter* Character);
	void HandleClueScanned(const UClueDataAsset* Clue);
	void RefreshObjectives();
	void ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds);

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> Objectives;

	UPROPERTY(Transient)
	TObjectPtr<USubtitleViewModel> Subtitles;

	FGothamSettingsListener SettingsListener;
	FTSTicker::FDelegateHandle SubtitleHideHandle;
};
