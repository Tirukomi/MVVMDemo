// Copyright IG. All Rights Reserved.

#include "Input/MvsBindings.h"

#include "Input/MvsActionTable.h"

#define LOCTEXT_NAMESPACE "Mvs.Keys"

namespace MvsBindings
{
	EMvsGamepadStyle GamepadStyleFromName(FName GamepadName)
	{
		const FString Name = GamepadName.ToString();
		if (Name.Contains(TEXT("PS")) || Name.Contains(TEXT("DualSense")) || Name.Contains(TEXT("DualShock")) || Name.Contains(TEXT("PlayStation")))
		{
			return EMvsGamepadStyle::PlayStation;
		}
		if (Name.Contains(TEXT("Switch")) || Name.Contains(TEXT("Nintendo")))
		{
			return EMvsGamepadStyle::Nintendo;
		}
		return EMvsGamepadStyle::Xbox;
	}

	FText GetKeyLabel(const FKey& Key, EMvsGamepadStyle Style)
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
		case EMvsGamepadStyle::PlayStation:
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
		case EMvsGamepadStyle::Nintendo:
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

	const TArray<FMvsBindingDef>& GetDefinitions()
	{
		static const TArray<FMvsBindingDef> Defs = []
		{
			TArray<FMvsBindingDef> Out;
			for (const FMvsActionDef& Action : MvsActions::GetTable())
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
		if (!Key.IsValid() || Key == EKeys::Escape || Key == EKeys::Virtual_Gamepad_Back.GetVirtualKey())
		{
			return false; // reserved: the menu back keys (UMvsUIInputData) cancel a rebind
		}
		if (Key.IsAxis1D() || Key.IsAxis2D() || Key.IsAxis3D())
		{
			return false; // sticks, triggers-as-axes and mouse motion are not button bindings
		}
		return Slot == GamepadSlot ? Key.IsGamepadKey() : !Key.IsGamepadKey();
	}

	TArray<FMvsBindingChange> PlanRebind(const TArray<FMvsBindingSlot>& Current, FName Name, int32 Slot, const FKey& NewKey)
	{
		TArray<FMvsBindingChange> Changes;
		if (!IsKeyAllowedForSlot(Slot, NewKey))
		{
			return Changes;
		}

		const FMvsBindingSlot* Target = Current.FindByPredicate([&](const FMvsBindingSlot& S) { return S.Name == Name && S.Slot == Slot; });
		if (!Target || Target->Key == NewKey)
		{
			return Changes;
		}

		Changes.Add({ Name, Slot, NewKey });

		// Another slot on the same device already uses the key: hand it the key we are giving up.
		for (const FMvsBindingSlot& Other : Current)
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
