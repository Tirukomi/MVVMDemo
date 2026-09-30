// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamBindings.h"

#include "Input/GothamActionTable.h"

namespace GothamBindings
{
	const TArray<FGothamBindingDef>& GetDefinitions()
	{
		static const TArray<FGothamBindingDef> Defs = []
		{
			TArray<FGothamBindingDef> Out;
			for (const FGothamActionDef& Action : GothamActions::GetTable())
			{
				if (Action.bRebindable)
				{
					Out.Add({ Action.Name, Action.DisplayName, Action.HasGamepadSlot() });
				}
			}
			return Out;
		}();
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

