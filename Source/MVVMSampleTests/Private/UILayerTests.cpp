// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "InputCoreTypes.h"
#include "Input/MvsActionTable.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUITypes.h"
#include "UI/Widgets/MvsInputGlyph.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsUITests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

// Only the HUD up: no menu. A menu or modal counts as a menu (pause, the wheel's key); the in-world layer does not.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsUIMenuOpenTest, "Mvs.UI.Layers.MenuOpen", MvsUITests::Flags)
bool FMvsUIMenuOpenTest::RunTest(const FString& Parameters)
{
	FMvsUIModeTracker Tracker;
	TestFalse("empty tracker has no menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EMvsUILayer::Game, true);
	TestFalse("HUD alone is not a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EMvsUILayer::GameMenu, true);
	TestFalse("the in-world overlay is not a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EMvsUILayer::Menu, true);
	TestTrue("the pause menu is a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EMvsUILayer::Menu, false);
	TestFalse("closing the menu leaves none", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EMvsUILayer::Modal, true);
	TestTrue("a modal alone is a menu too", Tracker.IsMenuOpen());
	return true;
}

// Back/cancel always closes the topmost thing, and never the HUD.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsUIDismissTest, "Mvs.UI.Layers.DismissOrder", MvsUITests::Flags)
bool FMvsUIDismissTest::RunTest(const FString& Parameters)
{
	FMvsUIModeTracker Tracker;
	Tracker.SetLayerOccupied(EMvsUILayer::Game, true);
	TestTrue("nothing to dismiss over the HUD", Tracker.GetTopDismissableLayer() == EMvsUILayer::Count);

	Tracker.SetLayerOccupied(EMvsUILayer::Menu, true);
	TestTrue("menu is dismissable", Tracker.GetTopDismissableLayer() == EMvsUILayer::Menu);

	Tracker.SetLayerOccupied(EMvsUILayer::Modal, true);
	TestTrue("modal closes before the menu beneath it", Tracker.GetTopDismissableLayer() == EMvsUILayer::Modal);

	Tracker.SetLayerOccupied(EMvsUILayer::Modal, false);
	TestTrue("then the menu", Tracker.GetTopDismissableLayer() == EMvsUILayer::Menu);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsGlyphLabelTest, "Mvs.UI.Glyphs.KeyLabels", MvsUITests::Flags)
bool FMvsGlyphLabelTest::RunTest(const FString& Parameters)
{
	TestEqual("gamepad bottom face is A", UMvsInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom).ToString(), FString(TEXT("A")));
	TestEqual("gamepad right face is B", UMvsInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Right).ToString(), FString(TEXT("B")));
	TestEqual("escape is abbreviated", UMvsInputGlyph::GetKeyLabel(EKeys::Escape).ToString(), FString(TEXT("Esc")));
	TestEqual("unmapped keys use the engine display name", UMvsInputGlyph::GetKeyLabel(EKeys::Q).ToString(), EKeys::Q.GetDisplayName().ToString());
	return true;
}

// Second review 9: the screens keys open are data. Each shortcut is a gameplay action the controller creates, opens a
// screen class the settings name, and appears once; a screen is opened by at most one key.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsUIShortcutsTest, "Mvs.UI.Shortcuts", MvsUITests::Flags)
bool FMvsUIShortcutsTest::RunTest(const FString& Parameters)
{
	const UMvsUISettings* Settings = GetDefault<UMvsUISettings>();
	TSet<FName> Actions;
	TSet<const void*> Screens;
	for (const FMvsScreenShortcut& Shortcut : UMvsUISettings::GetShortcuts())
	{
		const FString Name = Shortcut.Action.ToString();
		TestNotNull(*(Name + TEXT(" is a gameplay action")), MvsActions::Find(Shortcut.Action));
		TestFalse(*(Name + TEXT(" names a screen class")), (Settings->*Shortcut.Screen).IsNull());
		TestFalse(*(Name + TEXT(" is listed once")), Actions.Contains(Shortcut.Action));
		TestFalse(*(Name + TEXT(" opens a screen no other key opens")), Screens.Contains(&(Settings->*Shortcut.Screen)));
		TestTrue(*(Name + TEXT(" opens a screen above the HUD")), Shortcut.Layer != EMvsUILayer::Game && Shortcut.Layer != EMvsUILayer::Count);
		Actions.Add(Shortcut.Action);
		Screens.Add(&(Settings->*Shortcut.Screen));
		TestTrue(*(Name + TEXT(" is found by name")), UMvsUISettings::FindShortcut(Shortcut.Action) == &Shortcut);
	}
	TestTrue("pause, the case file and the gadget wheel open by key", Actions.Includes(TSet<FName>{ TEXT("Pause"), TEXT("ClueLog"), TEXT("GadgetWheel") }));
	TestNull("an action that opens nothing is not a shortcut", UMvsUISettings::FindShortcut(TEXT("Attack")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
