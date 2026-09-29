// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/** A rebindable action as shown on the controls screen. Each has a keyboard/mouse slot and optionally a gamepad slot. */
struct FGothamBindingDef
{
	FName Name;
	FText DisplayName;
	bool bHasGamepadSlot = true;
};

/** One key currently assigned to one (action, slot) pair. Slot 0 = keyboard/mouse, slot 1 = gamepad. */
struct FGothamBindingSlot
{
	FName Name;
	int32 Slot = 0;
	FKey Key;
};

/** A single assignment to make. */
struct FGothamBindingChange
{
	FName Name;
	int32 Slot = 0;
	FKey NewKey;
};

namespace GothamBindings
{
	inline constexpr int32 KeyboardSlot = 0;
	inline constexpr int32 GamepadSlot = 1;

	/** The rebindable actions, in display order. */
	MVVMSAMPLE_API const TArray<FGothamBindingDef>& GetDefinitions();

	/** Slot 0 accepts keyboard and mouse keys, slot 1 gamepad buttons. Escape / gamepad B are reserved to cancel. */
	MVVMSAMPLE_API bool IsKeyAllowedForSlot(int32 Slot, const FKey& Key);

	/**
	 * Works out what a rebind changes. Assigning a key already used by another slot of the same device swaps the two,
	 * so no key is ever left bound twice and nothing is silently lost. Returns an empty array for a no-op or an
	 * invalid request.
	 */
	MVVMSAMPLE_API TArray<FGothamBindingChange> PlanRebind(const TArray<FGothamBindingSlot>& Current, FName Name, int32 Slot, const FKey& NewKey);
}
