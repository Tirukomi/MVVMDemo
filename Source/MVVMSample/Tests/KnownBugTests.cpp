// Copyright Epic Games, Inc. All Rights Reserved.

// Review findings that are known bugs today (Docs/ReviewFixPlan.md). Each check records the bug without failing; once
// the behaviour is fixed the check fails with "KNOWN BUG FIXED", and the fix turns it into a normal assertion.
// The in-game ones are in Gotham.Functional.MenuInput.

#include "Misc/AutomationTest.h"

#include "Internationalization/Text.h"
#include "UI/Widgets/GothamInputGlyph.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamKnownBugTests
{
	void KnownBug(FAutomationTestBase& Test, bool bCorrect, const FString& Rule)
	{
		if (bCorrect)
		{
			Test.AddError(FString::Printf(TEXT("KNOWN BUG FIXED (promote to a rule): %s"), *Rule));
		}
		else
		{
			Test.AddInfo(FString::Printf(TEXT("KNOWN BUG: %s"), *Rule));
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamKnownBugKeyLabelsTest, "Gotham.KnownBugs.KeyLabelsLocalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamKnownBugKeyLabelsTest::RunTest(const FString& Parameters)
{
	// Review finding 5: short key names are shown on screen, so they must be localizable text (a namespace and key),
	// not FText::FromString.
	bool bAllLocalizable = true;
	for (const FKey& Key : { EKeys::Escape, EKeys::Enter, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_Special_Right, EKeys::Gamepad_Special_Left })
	{
		const FText Label = UGothamInputGlyph::GetKeyLabel(Key);
		bAllLocalizable &= FTextInspector::GetNamespace(Label).IsSet() && FTextInspector::GetKey(Label).IsSet();
	}
	GothamKnownBugTests::KnownBug(*this, bAllLocalizable, TEXT("on-screen key names are localizable (review 5)"));
	return true;
}

#endif
