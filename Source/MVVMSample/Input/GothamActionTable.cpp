// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamActionTable.h"

// Same namespace and keys as before the table existed, so the display names keep their translations.
#define LOCTEXT_NAMESPACE "Gotham.Bindings"

namespace GothamActions
{
	const TArray<FGothamActionDef>& GetTable()
	{
		using EType = EInputActionValueType;
		static const TArray<FGothamActionDef> Table = {
			// Name              Display text                                   Value          Keyboard / mouse         Gamepad                            Rebindable  Invert Y
			{ TEXT("MoveForward"), LOCTEXT("MoveForward", "Move forward"),     EType::Boolean, EKeys::W,                FKey(),                            true },
			{ TEXT("MoveBack"),    LOCTEXT("MoveBack", "Move back"),           EType::Boolean, EKeys::S,                FKey(),                            true },
			{ TEXT("MoveLeft"),    LOCTEXT("MoveLeft", "Move left"),           EType::Boolean, EKeys::A,                FKey(),                            true },
			{ TEXT("MoveRight"),   LOCTEXT("MoveRight", "Move right"),         EType::Boolean, EKeys::D,                FKey(),                            true },
			{ TEXT("Attack"),      LOCTEXT("Attack", "Attack"),                EType::Boolean, EKeys::LeftMouseButton,  EKeys::Gamepad_FaceButton_Bottom,  true },
			{ TEXT("Counter"),     LOCTEXT("Counter", "Counter"),              EType::Boolean, EKeys::RightMouseButton, EKeys::Gamepad_RightShoulder,      true },
			{ TEXT("Gadget1"),     LOCTEXT("Gadget1", "Gadget 1"),             EType::Boolean, EKeys::One,              EKeys::Gamepad_FaceButton_Left,    true },
			{ TEXT("Gadget2"),     LOCTEXT("Gadget2", "Gadget 2"),             EType::Boolean, EKeys::Two,              EKeys::Gamepad_FaceButton_Top,     true },
			{ TEXT("Gadget3"),     LOCTEXT("Gadget3", "Gadget 3"),             EType::Boolean, EKeys::Three,            EKeys::Gamepad_FaceButton_Right,   true },
			{ TEXT("GadgetWheel"), LOCTEXT("GadgetWheel", "Gadget wheel"),     EType::Boolean, EKeys::Q,                EKeys::Gamepad_LeftShoulder,       true },
			{ TEXT("Detective"),   LOCTEXT("Detective", "Detective mode"),     EType::Boolean, EKeys::V,                EKeys::Gamepad_DPad_Up,            true },
			{ TEXT("Scan"),        LOCTEXT("Scan", "Scan clue"),               EType::Boolean, EKeys::E,                EKeys::Gamepad_DPad_Right,         true },
			{ TEXT("ClueLog"),     LOCTEXT("ClueLog", "Case file"),            EType::Boolean, EKeys::J,                EKeys::Gamepad_Special_Left,       true },
			{ TEXT("Pause"),       LOCTEXT("Pause", "Pause"),                  EType::Boolean, EKeys::Escape,           EKeys::Gamepad_Special_Right,      true },
			// Fixed: sticks and mouse motion, and the dev shortcuts.
			{ TEXT("Move"),        FText(),                                    EType::Axis2D,  FKey(),                  EKeys::Gamepad_Left2D },
			{ TEXT("Look"),        FText(),                                    EType::Axis2D,  EKeys::Mouse2D,          EKeys::Gamepad_Right2D,            false,      true },
			{ TEXT("DebugDamage"), FText(),                                    EType::Boolean, EKeys::F1,               FKey() },
			{ TEXT("DebugHeal"),   FText(),                                    EType::Boolean, EKeys::F2,               FKey() },
		};
		return Table;
	}

	const FGothamActionDef* Find(FName Name)
	{
		return GetTable().FindByPredicate([Name](const FGothamActionDef& Def) { return Def.Name == Name; });
	}
}

#undef LOCTEXT_NAMESPACE
