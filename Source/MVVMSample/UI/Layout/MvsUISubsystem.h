// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/Layout/MvsUITypes.h"
#include "MvsUISubsystem.generated.h"

class UCommonActivatableWidget;
class UMvsPrimaryLayout;
struct FStreamableHandle;

/**
 * Owns the primary layout and is the one place screens are pushed and popped.
 * Gameplay and view models never talk to widgets; they ask this subsystem to show a screen.
 */
UCLASS()
class MVVMSAMPLE_API UMvsUISubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/**
	 * Creates the layout on first call (idempotent), and starts loading every screen class asynchronously
	 * (UMvsUISettings::GetPreloadPaths), so opening a screen later never loads on a key press.
	 */
	void EnsureLayout(APlayerController* Owner);

	UCommonActivatableWidget* PushScreen(EMvsUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass);
	/** Pushes a screen class from the UI settings (preloaded; see UMvsUISettings::Resolve). */
	UCommonActivatableWidget* PushScreen(EMvsUILayer Layer, const TSoftClassPtr<UCommonActivatableWidget>& ScreenClass);

	/** The screen showing in a layer, or null (no layout yet, an empty layer, or EMvsUILayer::Count). */
	UCommonActivatableWidget* GetActiveScreen(EMvsUILayer Layer) const;

	/** True once the preloaded screen classes are in memory. */
	bool AreScreensLoaded() const;

	/** Pushes a screen and lets the caller configure the instance before it activates. */
	template<typename TScreen>
	TScreen* PushScreen(EMvsUILayer Layer, TSubclassOf<TScreen> ScreenClass)
	{
		return Cast<TScreen>(PushScreen(Layer, TSubclassOf<UCommonActivatableWidget>(ScreenClass)));
	}

	/** Dismisses whatever is topmost above the HUD. Returns false if only the HUD is showing. */
	bool PopTopScreen();

	/** Opens the pause menu, or closes the topmost menu if one is already up. */
	void TogglePauseMenu();

	bool IsMenuOpen() const { return Tracker.IsMenuOpen(); }
	bool IsLayerOccupied(EMvsUILayer Layer) const { return Tracker.IsLayerOccupied(Layer); }

	/** Dev aid for profiling: hides or shows the entire UI layer. */
	void SetLayoutVisible(bool bVisible);

	/**
	 * The case file's key: closes the case file if it is the top menu, otherwise opens it on top of whatever menu is
	 * open (an open gadget wheel gives way first). A key only ever closes its own screen.
	 */
	void ToggleClueLog();

	/** Opens the hold-to-use gadget wheel unless it (or a menu) is already up. */
	void OpenGadgetWheel();

	/**
	 * True if Screen is in a layer stack with another screen pushed above it. Common UI deactivates a screen both when
	 * it is covered and when it is closed; this tells the two apart (a covered screen comes back).
	 */
	bool IsCovered(const UCommonActivatableWidget* Screen) const;

	/** True while any layer is animating between screens (Common UI blocks input to the layer meanwhile). */
	bool IsTransitioning() const { return TransitioningLayers != 0; }

private:
	void HandleLayerChanged(EMvsUILayer Layer);
	/**
	 * One background blur on screen: the topmost screen that blurs keeps its blur, the ones under it (a menu under a
	 * modal) turn theirs off, since the top blur already covers the world they would blur.
	 */
	void UpdateBackdrops();

	UPROPERTY(Transient)
	TObjectPtr<UMvsPrimaryLayout> Layout;

	/** Keeps the preloaded screen classes in memory for the session. */
	TSharedPtr<FStreamableHandle> ScreenClasses;

	FMvsUIModeTracker Tracker;
	/** One bit per EMvsUILayer that is mid-transition. */
	uint32 TransitioningLayers = 0;
};
