// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"

/**
 * One gameplay input action: the action the player controller creates (IA_<Name>), its default keys, and whether
 * players can rebind it. Rebindable actions appear on the Controls screen in table order, and saved rebinds refer
 * to their Name, so a rename loses players' rebinds (Mvs.Characterization.InputBindings pins the names).
 */
struct FMvsActionDef
{
	FName Name;
	/** Shown on the Controls screen. Empty for fixed actions. */
	FText DisplayName;
	EInputActionValueType ValueType = EInputActionValueType::Boolean;
	/** Default keyboard / mouse key (rebind slot 0). Invalid for none. */
	FKey KeyboardKey;
	/** Default gamepad key (rebind slot 1). Invalid for none. */
	FKey GamepadKey;
	bool bRebindable = false;
	/** Negate the Y axis (look: mouse and stick up looks up). */
	bool bInvertY = false;

	bool HasGamepadSlot() const { return bRebindable && GamepadKey.IsValid(); }
};

namespace MvsActions
{
	/** Every gameplay action: the rebindable ones first, in Controls-screen order, then the fixed ones. */
	MVVMSAMPLE_API const TArray<FMvsActionDef>& GetTable();
	MVVMSAMPLE_API const FMvsActionDef* Find(FName Name);
}
