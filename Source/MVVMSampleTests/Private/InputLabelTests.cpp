// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Input/MvsBindings.h"
#include "Internationalization/Text.h"

#if WITH_DEV_AUTOMATION_TESTS

// Short key names are shown on screen, so they must be localizable text (a namespace and a key, not FText::FromString),
// and gamepad buttons use the naming of the pad in hand (review findings 5).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsKeyLabelsTest, "Mvs.Input.KeyLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsKeyLabelsTest::RunTest(const FString& Parameters)
{
	for (const FKey& Key : { EKeys::Escape, EKeys::Enter, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_Special_Right, EKeys::Gamepad_Special_Left, EKeys::Gamepad_FaceButton_Bottom })
	{
		for (const EMvsGamepadStyle Style : { EMvsGamepadStyle::Xbox, EMvsGamepadStyle::PlayStation, EMvsGamepadStyle::Nintendo })
		{
			const FText Label = MvsBindings::GetKeyLabel(Key, Style);
			TestTrue(FString::Printf(TEXT("%s (style %d) is localizable"), *Key.ToString(), static_cast<int32>(Style)),
				FTextInspector::GetNamespace(Label).IsSet() && FTextInspector::GetKey(Label).IsSet());
		}
	}
	using EStyle = EMvsGamepadStyle;
	TestEqual("Xbox: bottom face is A", MvsBindings::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom, EStyle::Xbox).ToString(), FString(TEXT("A")));
	TestEqual("PlayStation: bottom face is Cross", MvsBindings::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom, EStyle::PlayStation).ToString(), FString(TEXT("Cross")));
	TestEqual("Nintendo: bottom face reads B", MvsBindings::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom, EStyle::Nintendo).ToString(), FString(TEXT("B")));
	TestEqual("PlayStation shoulder", MvsBindings::GetKeyLabel(EKeys::Gamepad_LeftShoulder, EStyle::PlayStation).ToString(), FString(TEXT("L1")));
	TestEqual("gamepad names pick a style", static_cast<int32>(MvsBindings::GamepadStyleFromName(TEXT("PS5"))), static_cast<int32>(EStyle::PlayStation));
	TestEqual("unknown pads are Xbox", static_cast<int32>(MvsBindings::GamepadStyleFromName(TEXT("Generic"))), static_cast<int32>(EStyle::Xbox));
	return true;
}

#endif
