// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamBindings.h"

#define LOCTEXT_NAMESPACE "Gotham.Bindings"

namespace GothamBindings
{
	const TArray<FGothamBindingDef>& GetDefinitions()
	{
		static const TArray<FGothamBindingDef> Defs = {
			{ TEXT("MoveForward"), LOCTEXT("MoveForward", "Move forward"), false },
			{ TEXT("MoveBack"), LOCTEXT("MoveBack", "Move back"), false },
			{ TEXT("MoveLeft"), LOCTEXT("MoveLeft", "Move left"), false },
			{ TEXT("MoveRight"), LOCTEXT("MoveRight", "Move right"), false },
			{ TEXT("Attack"), LOCTEXT("Attack", "Attack"), true },
			{ TEXT("Counter"), LOCTEXT("Counter", "Counter"), true },
			{ TEXT("Gadget1"), LOCTEXT("Gadget1", "Gadget 1"), true },
			{ TEXT("Gadget2"), LOCTEXT("Gadget2", "Gadget 2"), true },
			{ TEXT("Gadget3"), LOCTEXT("Gadget3", "Gadget 3"), true },
			{ TEXT("GadgetWheel"), LOCTEXT("GadgetWheel", "Gadget wheel"), true },
			{ TEXT("Detective"), LOCTEXT("Detective", "Detective mode"), true },
			{ TEXT("Scan"), LOCTEXT("Scan", "Scan clue"), true },
			{ TEXT("ClueLog"), LOCTEXT("ClueLog", "Case file"), true },
			{ TEXT("Pause"), LOCTEXT("Pause", "Pause"), true },
		};
		return Defs;
	}

	bool IsKeyAllowedForSlot(int32 Slot, const FKey& Key)
	{
		if (!Key.IsValid() || Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
		{
			return false; // reserved: these cancel a rebind
		}
		if (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())
		{
			return false; // sticks, triggers-as-axes and mouse motion are not button bindings
		}
		return Slot == GamepadSlot ? Key.IsGamepadKey() : !Key.IsGamepadKey();
	}

	TArray<FGothamBindingChange> PlanRebind(const TArray<FGothamBindingSlot>& Current, FName Name, int32 Slot, const FKey& NewKey)
	{
		TArray<FGothamBindingChange> Changes;
		if (!IsKeyAllowedForSlot(Slot, NewKey))
		{
			return Changes;
		}

		const FGothamBindingSlot* Target = Current.FindByPredicate([&](const FGothamBindingSlot& S) { return S.Name == Name && S.Slot == Slot; });
		if (!Target || Target->Key == NewKey)
		{
			return Changes;
		}

		Changes.Add({ Name, Slot, NewKey });

		// Another slot on the same device already uses the key: hand it the key we are giving up.
		for (const FGothamBindingSlot& Other : Current)
		{
			const bool bSameSlotKind = (Other.Slot == GamepadSlot) == (Slot == GamepadSlot);
			const bool bIsTarget = Other.Name == Name && Other.Slot == Slot;
			if (!bIsTarget && bSameSlotKind && Other.Key == NewKey)
			{
				Changes.Add({ Other.Name, Other.Slot, Target->Key });
			}
		}
		return Changes;
	}
}

#undef LOCTEXT_NAMESPACE
