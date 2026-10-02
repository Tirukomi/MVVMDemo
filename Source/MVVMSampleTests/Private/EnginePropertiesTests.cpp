// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Core/MvsEngineProperties.h"
#include "PlayerMappableKeySettings.h"
#include "UI/Widgets/MvsHintButton.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS

// Second review 16: the engine properties the game sets through reflection still exist, with a type the writes handle.
// An engine upgrade that renames one fails here, not as a silently ticking widget or an unrebindable action.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsEnginePropertiesTest, "Mvs.Engine.Properties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsEnginePropertiesTest::RunTest(const FString& Parameters)
{
	const FProperty* Tick = MvsEngineProperties::WidgetTickFrequency();
	TestTrue("UUserWidget::TickFrequency is an enum or a number (MvsUI::DisableTick)",
		Tick && (Tick->IsA<FEnumProperty>() || Tick->IsA<FNumericProperty>()));

	const FClassProperty* ButtonClass = MvsEngineProperties::ActionBarButtonClass();
	TestTrue("UCommonBoundActionBar::ActionButtonClass takes the prompt class (UMvsActionBar)",
		ButtonClass && ButtonClass->MetaClass && UMvsHintButton::StaticClass()->IsChildOf(ButtonClass->MetaClass));

	const FObjectProperty* KeySettings = MvsEngineProperties::InputActionKeySettings();
	TestTrue("UInputAction::PlayerMappableKeySettings takes a key settings object (rebindable actions)",
		KeySettings && KeySettings->PropertyClass && UPlayerMappableKeySettings::StaticClass()->IsChildOf(KeySettings->PropertyClass));
	return true;
}

#endif
