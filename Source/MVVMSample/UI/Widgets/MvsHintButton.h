// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "CommonButtonBase.h"
#include "Input/CommonBoundActionButtonInterface.h"
#include "InputCoreTypes.h"
#include "MvsHintButton.generated.h"

class UMvsInputGlyph;
class UInputAction;
class UTextBlock;

/**
 * A button prompt: the key glyph of an action (keyboard or gamepad, following the current device) and a label, as in
 * "[Esc] Back". Clicking it does what the key does, so mouse players never need the keyboard for menu actions.
 * Not focusable: pointing at it must not take focus (and the menu highlight) away from the current item.
 *
 * Used by UMvsActionBar for the actions a screen registers with Common UI (the action and the click both come from
 * the binding), or on its own with SetInputAction, where the owner binds OnClicked (the tab list's Q / E prompts).
 */
UCLASS()
class MVVMSAMPLE_API UMvsHintButton : public UCommonButtonBase, public ICommonBoundActionButtonInterface
{
	GENERATED_BODY()

public:
	UMvsHintButton(const FObjectInitializer& ObjectInitializer);

	/** Shows Action's key. Label may be empty (a bare key glyph, e.g. the tab prompts). */
	void SetInputAction(const UInputAction* InAction, const FText& InLabel);

	/** ICommonBoundActionButtonInterface: shows a registered binding and runs it when clicked. */
	virtual void SetRepresentedAction(FUIActionBindingHandle InBindingHandle) override;

	/** The keyboard key this prompt shows when the player uses mouse and keyboard (tests find prompts by it). */
	FKey GetKeyboardKey() const;

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;
	virtual void NativeOnClicked() override;

private:
	void ApplyState();

	UPROPERTY(Transient)
	TObjectPtr<UMvsInputGlyph> Glyph;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> Action;

	FUIActionBindingHandle BindingHandle;
	FText PendingLabel;
	bool bHoveredNow = false;
	bool bPressedNow = false;
	FMvsSettingsListener SettingsListener;
	/**
	 * What had keyboard focus when the pointer arrived. Clicking a non-focusable widget lets Slate move focus to the
	 * nearest focusable ancestor (the screen), so a click puts focus back before acting: "Select" then acts on the item
	 * that was current, and a gamepad picks up where it was.
	 */
	TWeakPtr<SWidget> FocusBeforePointer;
};
