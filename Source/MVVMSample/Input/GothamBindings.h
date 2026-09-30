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

/** Which naming a gamepad's buttons use on screen. */
enum class EGothamGamepadStyle : uint8
{
	Xbox,         // A B X Y, LB RB LT RT, View / Menu
	PlayStation,  // Cross Circle Square Triangle, L1 R1 L2 R2, Create / Options
	Nintendo,     // B A Y X by position, L R ZL ZR, - / +
};

namespace GothamBindings
{
	/** The style for a gamepad name as Common Input reports it ("PS5", "DualSense", "Switch" ...); Xbox otherwise. */
	MVVMSAMPLE_API EGothamGamepadStyle GamepadStyleFromName(FName GamepadName);

	/**
	 * A key's short on-screen name ("Esc", "LB", "D-pad Up"), localizable, in the given gamepad's naming. Keys without
	 * a short name use the engine's display name.
	 */
	MVVMSAMPLE_API FText GetKeyLabel(const FKey& Key, EGothamGamepadStyle Style = EGothamGamepadStyle::Xbox);

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
