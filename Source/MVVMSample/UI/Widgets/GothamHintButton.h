// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "InputCoreTypes.h"
#include "GothamHintButton.generated.h"

class UGothamInputGlyph;
class UTextBlock;

/**
 * A button prompt: the key glyph (keyboard or gamepad, following the current device) and a label, as in
 * "[Esc] Back". Clicking it does what the key does, so mouse players never need the keyboard for menu actions.
 * Not focusable: pointing at it must not take focus (and the menu highlight) away from the current item.
 */
UCLASS()
class MVVMSAMPLE_API UGothamHintButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UGothamHintButton(const FObjectInitializer& ObjectInitializer);

	/** Fixed keys, one per device family. Label may be empty (a bare key glyph, e.g. the tab prompts). */
	void SetHint(const FKey& InKeyboardKey, const FKey& InGamepadKey, const FText& InLabel);
	const FKey& GetKeyboardKey() const { return KeyboardKey; }

	/**
	 * What had keyboard focus when the pointer arrived. Clicking a non-focusable widget lets Slate move focus to the
	 * nearest focusable ancestor (the screen), so an "Accept" prompt restores this before acting on it.
	 */
	TSharedPtr<SWidget> GetFocusBeforePointer() const { return FocusBeforePointer.Pin(); }

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;

private:
	void ApplyState();

	UPROPERTY(Transient)
	TObjectPtr<UGothamInputGlyph> Glyph;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	FKey KeyboardKey;
	FKey GamepadKey;
	FText PendingLabel;
	bool bHoveredNow = false;
	bool bPressedNow = false;
	FDelegateHandle SettingsHandle;
	TWeakPtr<SWidget> FocusBeforePointer;
};
