// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "MvsViewModelSubsystem.generated.h"

class AMvsCharacter;
class UClueListViewModel;
class UComboViewModel;
class UForensicViewModel;
class UGadgetBarViewModel;
class UMvsClueBinder;
class UMvsViewModelBinder;
class UObjectivesViewModel;
class UPlayerVitalsViewModel;
class USubtitleViewModel;
class UThreatViewModel;

/**
 * The HUD view models of one local player, and the one place gameplay is wired into them. Each feature has its own
 * binder (ViewModels/MvsViewModelBinders.h) that owns the feature's view models and its subscriptions; this
 * subsystem holds the binders, binds them to the possessed character, and hands view models out by class.
 * Components and widgets never see each other.
 */
UCLASS()
class MVVMSAMPLE_API UMvsViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Binds every feature to the character (or only unbinds when null) and pushes its current state. */
	void BindToCharacter(AMvsCharacter* Character);

	/** The view model of a class (or a subclass), for the view-model resolver and for widgets. */
	UObject* FindViewModel(const UClass* ViewModelClass) const;
	template<typename TViewModel>
	TViewModel* Get() const { return Cast<TViewModel>(FindViewModel(TViewModel::StaticClass())); }

	UPlayerVitalsViewModel* GetVitals() const;
	UGadgetBarViewModel* GetGadgetBar() const;
	UComboViewModel* GetCombo() const;
	UForensicViewModel* GetForensic() const;
	UObjectivesViewModel* GetObjectives() const;
	UClueListViewModel* GetClues() const;
	USubtitleViewModel* GetSubtitles() const;
	UThreatViewModel* GetThreats() const;

#if !UE_BUILD_SHIPPING
	/** Dev aid: appends fake undiscovered clues (flagged IsDebug) to exercise the virtualised clue log. They never count
	 *  toward the objective, which tracks the level's real clues. Remove them by filtering the clue list on IsDebug. */
	void AddDebugClues(int32 Count);
#endif

private:
	template<typename TBinder>
	TBinder* AddBinder();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMvsViewModelBinder>> Binders;

	UPROPERTY(Transient)
	TObjectPtr<UMvsClueBinder> ClueBinder;
};
