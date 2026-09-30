// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamBindings.h"

#include "Input/GothamActionTable.h"

#define LOCTEXT_NAMESPACE "Gotham.Keys"

namespace GothamBindings
{
	EGothamGamepadStyle GamepadStyleFromName(FName GamepadName)
	{
		const FString Name = GamepadName.ToString();
		if (Name.Contains(TEXT("PS")) || Name.Contains(TEXT("DualSense")) || Name.Contains(TEXT("DualShock")) || Name.Contains(TEXT("PlayStation")))
		{
			return EGothamGamepadStyle::PlayStation;
		}
		if (Name.Contains(TEXT("Switch")) || Name.Contains(TEXT("Nintendo")))
		{
			return EGothamGamepadStyle::Nintendo;
		}
		return EGothamGamepadStyle::Xbox;
	}

	FText GetKeyLabel(const FKey& Key, EGothamGamepadStyle Style)
	{
		// Device-independent keys first.
		if (Key == EKeys::Escape)             { return LOCTEXT("Esc", "Esc"); }
		if (Key == EKeys::Enter)              { return LOCTEXT("Enter", "Enter"); }
		if (Key == EKeys::LeftMouseButton)    { return LOCTEXT("LMB", "LMB"); }
		if (Key == EKeys::RightMouseButton)   { return LOCTEXT("RMB", "RMB"); }
		if (Key == EKeys::MiddleMouseButton)  { return LOCTEXT("MMB", "MMB"); }
		if (Key == EKeys::Gamepad_DPad_Up)    { return LOCTEXT("DPadUp", "D-pad Up"); }
		if (Key == EKeys::Gamepad_DPad_Down)  { return LOCTEXT("DPadDown", "D-pad Down"); }
		if (Key == EKeys::Gamepad_DPad_Left)  { return LOCTEXT("DPadLeft", "D-pad Left"); }
		if (Key == EKeys::Gamepad_DPad_Right) { return LOCTEXT("DPadRight", "D-pad Right"); }

		switch (Style)
		{
		case EGothamGamepadStyle::PlayStation:
			if (Key == EKeys::Gamepad_FaceButton_Bottom) { return LOCTEXT("PSCross", "Cross"); }
			if (Key == EKeys::Gamepad_FaceButton_Right)  { return LOCTEXT("PSCircle", "Circle"); }
			if (Key == EKeys::Gamepad_FaceButton_Left)   { return LOCTEXT("PSSquare", "Square"); }
			if (Key == EKeys::Gamepad_FaceButton_Top)    { return LOCTEXT("PSTriangle", "Triangle"); }
			if (Key == EKeys::Gamepad_LeftShoulder)      { return LOCTEXT("PSL1", "L1"); }
			if (Key == EKeys::Gamepad_RightShoulder)     { return LOCTEXT("PSR1", "R1"); }
			if (Key == EKeys::Gamepad_LeftTrigger)       { return LOCTEXT("PSL2", "L2"); }
			if (Key == EKeys::Gamepad_RightTrigger)      { return LOCTEXT("PSR2", "R2"); }
			if (Key == EKeys::Gamepad_Special_Left)      { return LOCTEXT("PSCreate", "Create"); }
			if (Key == EKeys::Gamepad_Special_Right)     { return LOCTEXT("PSOptions", "Options"); }
			break;
		case EGothamGamepadStyle::Nintendo:
			// Named by what is printed at each position: the bottom button reads B, the right one A.
			if (Key == EKeys::Gamepad_FaceButton_Bottom) { return LOCTEXT("NintendoB", "B"); }
			if (Key == EKeys::Gamepad_FaceButton_Right)  { return LOCTEXT("NintendoA", "A"); }
			if (Key == EKeys::Gamepad_FaceButton_Left)   { return LOCTEXT("NintendoY", "Y"); }
			if (Key == EKeys::Gamepad_FaceButton_Top)    { return LOCTEXT("NintendoX", "X"); }
			if (Key == EKeys::Gamepad_LeftShoulder)      { return LOCTEXT("NintendoL", "L"); }
			if (Key == EKeys::Gamepad_RightShoulder)     { return LOCTEXT("NintendoR", "R"); }
			if (Key == EKeys::Gamepad_LeftTrigger)       { return LOCTEXT("NintendoZL", "ZL"); }
			if (Key == EKeys::Gamepad_RightTrigger)      { return LOCTEXT("NintendoZR", "ZR"); }
			if (Key == EKeys::Gamepad_Special_Left)      { return LOCTEXT("NintendoMinus", "-"); }
			if (Key == EKeys::Gamepad_Special_Right)     { return LOCTEXT("NintendoPlus", "+"); }
			break;
		default:
			if (Key == EKeys::Gamepad_FaceButton_Bottom) { return LOCTEXT("XboxA", "A"); }
			if (Key == EKeys::Gamepad_FaceButton_Right)  { return LOCTEXT("XboxB", "B"); }
			if (Key == EKeys::Gamepad_FaceButton_Left)   { return LOCTEXT("XboxX", "X"); }
			if (Key == EKeys::Gamepad_FaceButton_Top)    { return LOCTEXT("XboxY", "Y"); }
			if (Key == EKeys::Gamepad_LeftShoulder)      { return LOCTEXT("XboxLB", "LB"); }
			if (Key == EKeys::Gamepad_RightShoulder)     { return LOCTEXT("XboxRB", "RB"); }
			if (Key == EKeys::Gamepad_LeftTrigger)       { return LOCTEXT("XboxLT", "LT"); }
			if (Key == EKeys::Gamepad_RightTrigger)      { return LOCTEXT("XboxRT", "RT"); }
			if (Key == EKeys::Gamepad_Special_Left)      { return LOCTEXT("XboxView", "View"); }
			if (Key == EKeys::Gamepad_Special_Right)     { return LOCTEXT("XboxMenu", "Menu"); }
			break;
		}
		return Key.GetDisplayName();
	}

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

#undef LOCTEXT_NAMESPACE
