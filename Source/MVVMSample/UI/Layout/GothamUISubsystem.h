// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/Layout/GothamUITypes.h"
#include "GothamUISubsystem.generated.h"

class UCommonActivatableWidget;
class UGothamPrimaryLayout;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGothamInputContextChanged, EGothamInputContext);

/**
 * Owns the primary layout and is the one place screens are pushed and popped.
 * Gameplay and view models never talk to widgets; they ask this subsystem to show a screen.
 */
UCLASS()
class MVVMSAMPLE_API UGothamUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Creates the layout on first call (idempotent). */
	void EnsureLayout(APlayerController* Owner);

	UCommonActivatableWidget* PushScreen(EGothamUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);

	/** Pushes a screen and lets the caller configure the instance before it activates. */
	template<typename TScreen>
	TScreen* PushScreen(EGothamUILayer Layer, TSubclassOf<TScreen> ScreenClass)
	{
		return Cast<TScreen>(PushScreen(Layer, TSubclassOf<UCommonActivatableWidget>(ScreenClass)));
	}

	/** Dismisses whatever is topmost above the HUD. Returns false if only the HUD is showing. */
	bool PopTopScreen();

	/** Opens the pause menu, or closes the topmost menu if one is already up. */
	void TogglePauseMenu();

	bool IsMenuOpen() const { return Tracker.IsMenuOpen(); }
	bool IsLayerOccupied(EGothamUILayer Layer) const { return Tracker.IsLayerOccupied(Layer); }

	/** Dev aid for profiling: hides or shows the entire UI layer. */
	void SetLayoutVisible(bool bVisible);

	/** Opens the case file, or closes the topmost menu if one is already up. */
	void ToggleClueLog();

	/** Opens the hold-to-use gadget wheel unless it (or a menu) is already up. */
	void OpenGadgetWheel();
	EGothamInputContext GetInputContext() const { return Tracker.GetInputContext(); }

	FOnGothamInputContextChanged OnInputContextChanged;

	/** Fired after the player rebinds controls, so glyphs and hints re-read their keys. */
	FSimpleMulticastDelegate OnBindingsChanged;
	void NotifyBindingsChanged() { OnBindingsChanged.Broadcast(); }

private:
	void HandleLayerChanged(EGothamUILayer Layer);

	UPROPERTY(Transient)
	TObjectPtr<UGothamPrimaryLayout> Layout;

	FGothamUIModeTracker Tracker;
};
