// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "CommonActivatableWidget.h"
#include "Input/UIActionBindingHandle.h"
#include "UI/Style/GothamStyle.h"
#include "GothamScreen.generated.h"

class UGothamButton;
class UGothamMenuList;
class UInputAction;
class UTextBlock;
class UVerticalBox;

/**
 * Base for every full screen and modal. Gives menus their input config (cursor, UI-only input) and default focus so
 * gamepad navigation always has somewhere to start.
 *
 * Input is Common UI's: Back (Esc, or the platform's back button) is the activatable back handler, and every other key
 * a screen answers to is a UI action binding, so keys follow rebinding and the prompts in the action bar always match
 * what the keys do. Bindings belong to the screen and only fire while it is the active one.
 *
 * Also owns the shared menu look: BuildMenuFrame gives a blurred, darkened world with a left-aligned column under a
 * section label and title, and the frame slides in on activation (the layer stack adds the fade in and out).
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UGothamScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGothamScreen(const FObjectInitializer& ObjectInitializer);

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override final;

	/** Another screen was pushed on top; this one comes back when that closes. */
	virtual void NativeOnCovered() {}
	/** This screen is leaving for good (back, close, or popped). What should happen once per visit goes here. */
	virtual void NativeOnClosed() {}
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** Re-apply palette-dependent colours (called on construct and whenever settings change). */
	virtual void OnPaletteChanged();

	/** Whether other screens' keys (the case file's) open those screens on top of this one. Off for modals. */
	bool bOpensScreensByKey = true;

	/**
	 * The gameplay action that opens this screen (e.g. "ClueLog"). Its keys close the screen again, so the same button
	 * toggles it, however the player has bound it.
	 */
	FName ToggleActionName;

	/** A gameplay action by name (AGothamPlayerController::FindAction), for screens that bind gameplay keys. */
	const UInputAction* FindGameplayAction(FName ActionName) const;

	/** Registers a key binding on this screen (not shown in the action bar). Call from NativeConstruct, once. */
	FUIActionBindingHandle BindAction(const UInputAction* Action, EInputEvent Event, FSimpleDelegate Handler);

	/** Set by subclasses to the widget that should receive focus when the screen opens. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> DefaultFocus;

	/** Slides in on activation (usually the frame built by BuildMenuFrame). */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> SlideTarget;

	/** Where SlideTarget starts its intro, relative to its layout position (default: from the left). */
	FVector2D SlideFrom;

	/**
	 * Builds the root of a full-screen menu and returns the content column (it fills the space between the header and
	 * the footer). Section is the small accent label above the title.
	 */
	UVerticalBox* BuildMenuFrame(const FText& Section, const FText& Title, float BlurStrength = 12.f);
	/** Puts Footer (usually the hint bar) at the bottom right of a frame built by BuildMenuFrame. */
	void AddFooter(UWidget* Footer);

	// Small builders so screens stay short and consistent.
	/** A text block in a type-scale style, coloured from a palette token (kept in sync with settings). */
	UTextBlock* MakeText(const FText& Text, EGothamTextStyle Style, EGothamColorToken Color);
	/** A big left-aligned menu item in a highlight list. */
	UGothamButton* AddMenuItem(UGothamMenuList* List, const FText& Label) const;
	/**
	 * The "[Enter/A] Select   [Esc/B] Back" prompt row: a bound action bar, with this screen's labels for the accept and
	 * back actions. Each prompt is also a button that does what its key does. Call while building the screen.
	 */
	UWidget* MakeActionBar(const FText& InAcceptLabel, const FText& BackLabel);

private:
	/** What accept does when it reaches the screen (the prompt was clicked): accepts the focused item. */
	void AcceptFocused();

	/** Shown on the accept prompt; empty for screens without one. */
	FText AcceptLabel;

	FUIActionBindingHandle AcceptHandle;
	FUIActionBindingHandle ToggleHandle;
	FUIActionBindingHandle ClueLogHandle;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> FrameBox;

	/** Text blocks made by MakeText, with the token each one uses. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> TokenTexts;
	TArray<EGothamColorToken> TokenTextColors;

	/** Accent pieces of the frame header, recoloured with the palette. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> AccentBars;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> HeaderRule;

	FGothamSettingsListener SettingsListener;
};
