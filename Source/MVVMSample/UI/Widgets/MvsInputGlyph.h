// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "CommonUserWidget.h"
#include "InputCoreTypes.h"
#include "MvsInputGlyph.generated.h"

class APlayerController;
class ULocalPlayer;
class UInputAction;
class UTextBlock;
enum class ECommonInputType : uint8;

/**
 * Shows the key or button that triggers an Enhanced Input action, as a key cap with its short name, and swaps live
 * when the player changes device or rebinds. The key is the one Common UI would use for the current device (the first
 * key of that device mapped to the action), so a prompt and the action it names always agree.
 *
 * Key caps are drawn from localized labels rather than Common Input's per-controller icon brushes: the project ships
 * no icon art, and text caps follow UI scale, high contrast and language like every other label.
 */
UCLASS()
class MVVMSAMPLE_API UMvsInputGlyph : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** Follow a gameplay action by name (asked of the owning player controller, IMvsActionSource). */
	void SetAction(FName InActionName);

	/** Follow any action, e.g. one of the menu actions in UMvsUIInputData. */
	void SetInputAction(const UInputAction* InAction);

	/** The key's short name in the naming of the gamepad Player is using (Xbox if unknown). See MvsBindings::GetKeyLabel. */
	static FText GetKeyLabel(const FKey& Key, const ULocalPlayer* Player = nullptr);

	/** The key Common UI shows for Action on the device Player is using now (invalid if none is mapped). */
	static FKey FindKey(const ULocalPlayer* Player, const UInputAction* Action);
	/** The same for one device. */
	static FKey FindKey(const ULocalPlayer* Player, const UInputAction* Action, ECommonInputType InputType);

	/** FindKey for a gameplay action by name. */
	static FKey FindKeyForAction(const APlayerController* Player, FName InActionName);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void HandleInputMethodChanged(ECommonInputType NewType) { Refresh(); }

	/** Enhanced Input rebuilt its key mappings (first build, rebinding, a context added or removed). */
	UFUNCTION()
	void HandleMappingsRebuilt() { Refresh(); }

	FName ActionName;

	UPROPERTY(Transient)
	TObjectPtr<const UInputAction> Action;

	FDelegateHandle InputMethodHandle;
	FMvsSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text;

	UPROPERTY(Transient)
	TObjectPtr<class UMvsPanel> Frame;
};
