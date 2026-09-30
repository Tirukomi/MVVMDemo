// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "CommonActivatableWidget.h"
#include "UI/Style/GothamStyle.h"
#include "GothamScreen.generated.h"

class UGothamButton;
class UGothamHintButton;
class UGothamInputGlyph;
class UGothamMenuList;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;

/**
 * Base for every full screen and modal. Gives menus their input config (cursor, UI-only input),
 * Esc / gamepad B to go back, and default focus so gamepad navigation always has somewhere to start.
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
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Re-apply palette-dependent colours (called on construct and whenever settings change). */
	virtual void OnPaletteChanged();

	/** Screens that must be answered (e.g. confirmations that block) can turn back off. */
	bool bCanDismissWithBack = true;

	/**
	 * The gameplay action that opens this screen (e.g. "ClueLog"). Pressing any key bound to it closes the screen
	 * again, so the same button toggles it. Follows rebinding: the keys come from the live Enhanced Input mapping.
	 */
	FName ToggleActionName;

	/** What the Back prompt and Esc / B do. Default: close the screen (if it may be dismissed). */
	virtual void HandleBack();
	/** What the Select prompt does when clicked: the same as pressing Enter on Target (what had focus before the
	 *  pointer went to the prompt), or on whatever has focus if Target is gone. */
	void HandleAccept(TSharedPtr<SWidget> Target);
	/** True if Key is currently bound to the named gameplay action (either device). */
	bool IsKeyBoundToAction(const FKey& Key, FName ActionName) const;

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
	/** "[Enter/A] Select   [Esc/B] Back" prompt row. Each prompt is also a button that does what its key does. */
	UHorizontalBox* MakeHintBar(const FText& AcceptLabel, const FText& BackLabel);
	/** Adds one more clickable prompt to a hint bar; bind its OnClicked() to the action. */
	UGothamHintButton* AddHint(UHorizontalBox* Bar, const FKey& Keyboard, const FKey& Pad, const FText& Label);

private:
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
