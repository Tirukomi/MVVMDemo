// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "InputCoreTypes.h"
#include "UI/Layout/GothamUITypes.h"
#include "UI/Widgets/GothamInputGlyph.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamUITests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

// Only the HUD up: no menu. A menu or modal counts as a menu (pause, the wheel's key); the in-world layer does not.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamUIMenuOpenTest, "Gotham.UI.Layers.MenuOpen", GothamUITests::Flags)
bool FGothamUIMenuOpenTest::RunTest(const FString& Parameters)
{
	FGothamUIModeTracker Tracker;
	TestFalse("empty tracker has no menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EGothamUILayer::Game, true);
	TestFalse("HUD alone is not a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EGothamUILayer::GameMenu, true);
	TestFalse("the in-world overlay is not a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EGothamUILayer::Menu, true);
	TestTrue("the pause menu is a menu", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EGothamUILayer::Menu, false);
	TestFalse("closing the menu leaves none", Tracker.IsMenuOpen());

	Tracker.SetLayerOccupied(EGothamUILayer::Modal, true);
	TestTrue("a modal alone is a menu too", Tracker.IsMenuOpen());
	return true;
}

// Back/cancel always closes the topmost thing, and never the HUD.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamUIDismissTest, "Gotham.UI.Layers.DismissOrder", GothamUITests::Flags)
bool FGothamUIDismissTest::RunTest(const FString& Parameters)
{
	FGothamUIModeTracker Tracker;
	Tracker.SetLayerOccupied(EGothamUILayer::Game, true);
	TestTrue("nothing to dismiss over the HUD", Tracker.GetTopDismissableLayer() == EGothamUILayer::Count);

	Tracker.SetLayerOccupied(EGothamUILayer::Menu, true);
	TestTrue("menu is dismissable", Tracker.GetTopDismissableLayer() == EGothamUILayer::Menu);

	Tracker.SetLayerOccupied(EGothamUILayer::Modal, true);
	TestTrue("modal closes before the menu beneath it", Tracker.GetTopDismissableLayer() == EGothamUILayer::Modal);

	Tracker.SetLayerOccupied(EGothamUILayer::Modal, false);
	TestTrue("then the menu", Tracker.GetTopDismissableLayer() == EGothamUILayer::Menu);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamGlyphLabelTest, "Gotham.UI.Glyphs.KeyLabels", GothamUITests::Flags)
bool FGothamGlyphLabelTest::RunTest(const FString& Parameters)
{
	TestEqual("gamepad bottom face is A", UGothamInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom).ToString(), FString(TEXT("A")));
	TestEqual("gamepad right face is B", UGothamInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Right).ToString(), FString(TEXT("B")));
	TestEqual("escape is abbreviated", UGothamInputGlyph::GetKeyLabel(EKeys::Escape).ToString(), FString(TEXT("Esc")));
	TestEqual("unmapped keys use the engine display name", UGothamInputGlyph::GetKeyLabel(EKeys::Q).ToString(), EKeys::Q.GetDisplayName().ToString());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
