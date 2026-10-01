// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "InputCoreTypes.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
