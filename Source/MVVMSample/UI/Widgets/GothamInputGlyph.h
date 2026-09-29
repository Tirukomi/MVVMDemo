// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Containers/Ticker.h"
#include "InputCoreTypes.h"
#include "GothamInputGlyph.generated.h"

class UInputAction;
class UTextBlock;
enum class ECommonInputType : uint8;

/**
 * Shows the key or button that triggers something, and swaps live when the player changes device.
 * Either follows an Enhanced Input action (so rebinding is reflected) or shows fixed hint keys.
 */
UCLASS()
class MVVMSAMPLE_API UGothamInputGlyph : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Follow a gameplay action by name (see AGothamPlayerController::FindAction). */
	void SetAction(FName InActionName);

	/** Show fixed hint keys, one per device family (used for UI navigation prompts). */
	void SetFixedKeys(FKey InKeyboardMouseKey, FKey InGamepadKey);

	/** Short label for a key, e.g. "Esc", "A". Static so tests can cover the mapping. */
	static FText GetKeyLabel(const FKey& Key);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void HandleInputMethodChanged(ECommonInputType NewType) { Refresh(); }

	/** Mapping queries return nothing until Enhanced Input rebuilds its mappings next tick; retry briefly. */
	void ScheduleRetry();
	int32 RetriesLeft = 10;
	FTSTicker::FDelegateHandle RetryHandle;

	FName ActionName;
	FKey FixedKeyboardMouse;
	FKey FixedGamepad;
	FDelegateHandle InputMethodHandle;
	FDelegateHandle BindingsHandle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text;

	UPROPERTY(Transient)
	TObjectPtr<class UGothamPanel> Frame;
};
