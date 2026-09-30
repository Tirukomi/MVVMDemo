// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GothamViewModelSubsystem.generated.h"

class AGothamCharacter;
class UComboComponent;
class UClueListViewModel;
class UComboViewModel;
class UDetectiveComponent;
class UDetectiveViewModel;
class UObjectivesViewModel;
class USubtitleViewModel;
class UThreatViewModel;
class UGothamThreatSubsystem;
struct FGothamSettingsData;
class UClueDataAsset;
class UGadgetBarViewModel;
class UGadgetComponent;
class UHealthComponent;
class UPlayerVitalsViewModel;

/**
 * Owns the HUD view models for one local player and wires gameplay components into them.
 * This is the only place that knows about both sides; components and widgets never see each other.
 */
UCLASS()
class MVVMSAMPLE_API UGothamViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Subscribes to the character's components (or unsubscribes when null) and pushes initial state. */
	void BindToCharacter(AGothamCharacter* Character);

	UPlayerVitalsViewModel* GetVitals() const { return Vitals; }
	UGadgetBarViewModel* GetGadgetBar() const { return GadgetBar; }
	UComboViewModel* GetCombo() const { return Combo; }
	UDetectiveViewModel* GetDetective() const { return Detective; }
	UObjectivesViewModel* GetObjectives() const { return Objectives; }
	UClueListViewModel* GetClues() const { return Clues; }
	USubtitleViewModel* GetSubtitles() const { return Subtitles; }
	UThreatViewModel* GetThreats() const { return Threats; }

	/** Dev aid: appends fake undiscovered clues to exercise the virtualised clue log. */
	void AddDebugClues(int32 Count);

	/** Used by the view-model resolver to hand a view model to a widget by class. */
	UObject* FindViewModel(const UClass* ViewModelClass) const;

private:
	void Unbind();

	void HandleHealth(float Health, float MaxHealth);
	void HandleGadgetCooldown(int32 Slot, float Remaining, float Total);
	void HandleCombo(int32 Hits, float Multiplier, float DecayAlpha);
	void HandleDetective(bool bActive, float Alpha);
	void HandleClueScanned(const UClueDataAsset* Clue);
	void RefreshObjectives();
	void RebuildClues();
	void HandleSettings(const FGothamSettingsData& Data);
	void ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds);

	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> Vitals;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBar;

	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> Combo;

	UPROPERTY(Transient)
	TObjectPtr<UThreatViewModel> Threats;

	TWeakObjectPtr<UGothamThreatSubsystem> BoundThreats;
	FDelegateHandle ThreatsHandle;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> Detective;

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> Objectives;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<USubtitleViewModel> Subtitles;

	TWeakObjectPtr<UHealthComponent> BoundHealth;
	TWeakObjectPtr<UGadgetComponent> BoundGadgets;
	TWeakObjectPtr<UComboComponent> BoundCombo;
	TWeakObjectPtr<UDetectiveComponent> BoundDetective;

	FDelegateHandle HealthHandle;
	FDelegateHandle GadgetHandle;
	FDelegateHandle GadgetUsedHandle;
	FDelegateHandle ComboHandle;
	FDelegateHandle DetectiveHandle;
	FDelegateHandle ScanHandle;
	FDelegateHandle AnalysisHandle;
	FDelegateHandle CluesHandle;
	FDelegateHandle SettingsHandle;
	FTSTicker::FDelegateHandle SubtitleHideHandle;
};
