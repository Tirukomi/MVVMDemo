// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTypes.h"
#include "UI/Slate/SComboMeter.h"
#include "UI/Slate/SGadgetIcon.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Style/MvsMotion.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/PlayerVitalsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsHudTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

// A drop holds, then drains to the new value; a rise is immediate; repeated hits restart the hold.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsGhostFillTest, "Mvs.HUD.GhostFill", MvsHudTests::Flags)
bool FMvsGhostFillTest::RunTest(const FString& Parameters)
{
	FMvsGhostFill Ghost;
	Ghost.SetTarget(1.f);
	TestEqual("a rise is immediate", Ghost.Ghost, 1.f);

	Ghost.SetTarget(0.6f);
	TestTrue("still busy right after a drop", Ghost.Advance(0.6f, 0.1f));
	TestEqual("holds at the old value during the hold", Ghost.Ghost, 1.f);

	Ghost.Advance(0.6f, FMvsGhostFill::HoldSeconds);
	Ghost.Advance(0.6f, 0.1f);
	TestTrue("then drains", Ghost.Ghost < 1.f && Ghost.Ghost > 0.6f);

	Ghost.SetTarget(0.4f);
	const float BeforeHold = Ghost.Ghost;
	Ghost.Advance(0.4f, 0.2f);
	TestEqual("a new hit restarts the hold", Ghost.Ghost, BeforeHold);

	for (int32 i = 0; i < 100; ++i)
	{
		Ghost.Advance(0.4f, 0.05f);
	}
	TestEqual("settles on the target", Ghost.Ghost, 0.4f);
	TestFalse("and stops animating", Ghost.Advance(0.4f, 0.05f));

	Ghost.SetTarget(0.9f);
	TestEqual("healing above the ghost moves it up at once", Ghost.Ghost, 0.9f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsChamferTest, "Mvs.HUD.ChamferShape", MvsHudTests::Flags)
bool FMvsChamferTest::RunTest(const FString& Parameters)
{
	const FVector2f Size(200.f, 60.f);
	TestEqual("no chamfer is a rectangle", MvsChamferedRect(Size, 10.f, EMvsChamfer::None).Num(), 4);
	TestEqual("two cut corners add two points", MvsChamferedRect(Size, 10.f, EMvsChamfer::Opposite).Num(), 6);
	TestEqual("all corners cut is an octagon", MvsChamferedRect(Size, 10.f, EMvsChamfer::All).Num(), 8);

	for (const FVector2f& P : MvsChamferedRect(Size, 500.f, EMvsChamfer::All))
	{
		TestTrue("an oversized corner is clamped inside the box", P.X >= 0.f && P.X <= Size.X && P.Y >= 0.f && P.Y <= Size.Y);
	}
	const TArray<FVector2f> Cut = MvsChamferedRect(Size, 10.f, EMvsChamfer::TopRight);
	TestTrue("top-right cut starts 10px before the corner", Cut.Contains(FVector2f(190.f, 0.f)) && Cut.Contains(FVector2f(200.f, 10.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsPunchTest, "Mvs.HUD.PunchCurve", MvsHudTests::Flags)
bool FMvsPunchTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual("starts at rest", MvsMotion::PunchCurve(0.f), 0.f, 0.001f);
	TestNearlyEqual("peaks early", MvsMotion::PunchCurve(0.25f), 1.f, 0.001f);
	TestNearlyEqual("ends at rest", MvsMotion::PunchCurve(1.f), 0.f, 0.001f);
	TestTrue("rises faster than it falls", MvsMotion::PunchCurve(0.15f) > MvsMotion::PunchCurve(0.65f));
	TestNearlyEqual("clamps beyond the end", MvsMotion::PunchCurve(3.f), 0.f, 0.001f);
	return true;
}

// Neutral tokens stay legible: text reads against panels in every mode, and high contrast only increases contrast.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsNeutralTokenTest, "Mvs.Accessibility.NeutralTokens", MvsHudTests::Flags)
bool FMvsNeutralTokenTest::RunTest(const FString& Parameters)
{
	auto Luma = [](const FLinearColor& C) { return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B; };
	for (const bool bHigh : { false, true })
	{
		const float Panel = Luma(MvsPalette::Resolve(EMvsColorToken::Panel, EMvsColorMode::Default, bHigh));
		const float Primary = Luma(MvsPalette::Resolve(EMvsColorToken::TextPrimary, EMvsColorMode::Default, bHigh));
		const float Muted = Luma(MvsPalette::Resolve(EMvsColorToken::TextMuted, EMvsColorMode::Default, bHigh));
		// WCAG-style contrast ratio (L1 + 0.05) / (L2 + 0.05) in linear light.
		TestTrue(*FString::Printf(TEXT("primary text >= 7:1 on panels (contrast %d)"), bHigh), (Primary + 0.05f) / (Panel + 0.05f) >= 7.f);
		TestTrue(*FString::Printf(TEXT("muted text >= 4.5:1 on panels (contrast %d)"), bHigh), (Muted + 0.05f) / (Panel + 0.05f) >= 4.5f);
	}
	for (int32 Mode = 0; Mode < static_cast<int32>(EMvsColorMode::Count); ++Mode)
	{
		TestTrue("neutral tokens ignore the colour-vision preset",
			MvsPalette::Resolve(EMvsColorToken::Accent, static_cast<EMvsColorMode>(Mode), false)
				.Equals(MvsPalette::Resolve(EMvsColorToken::Accent, EMvsColorMode::Default, false)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsDamageCountTest, "Mvs.ViewModels.Vitals.DamageEvents", MvsHudTests::Flags)
bool FMvsDamageCountTest::RunTest(const FString& Parameters)
{
	UPlayerVitalsViewModel* VM = NewObject<UPlayerVitalsViewModel>(GetTransientPackage());
	VM->SetVitals(100.f, 100.f);
	TestEqual("the first value is not damage", VM->GetDamageCount(), 0);
	VM->SetVitals(80.f, 100.f);
	VM->SetVitals(60.f, 100.f);
	TestEqual("each drop counts", VM->GetDamageCount(), 2);
	VM->SetVitals(90.f, 100.f);
	VM->SetVitals(90.f, 100.f);
	TestEqual("heals and repeats do not count", VM->GetDamageCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsGadgetSelectionTest, "Mvs.ViewModels.Gadgets.Selection", MvsHudTests::Flags)
bool FMvsGadgetSelectionTest::RunTest(const FString& Parameters)
{
	UGadgetBarViewModel* Bar = NewObject<UGadgetBarViewModel>(GetTransientPackage());
	Bar->SetSlotCount(3);
	TestEqual("defaults to the first gadget", Bar->GetSelectedIndex(), 0);
	Bar->SetSelectedIndex(2);
	TestEqual("selects a valid slot", Bar->GetSelectedIndex(), 2);
	Bar->SetSelectedIndex(7);
	TestEqual("ignores an invalid slot", Bar->GetSelectedIndex(), 2);

	Bar->GetSlot(1)->SetDefinition(FText::FromString(TEXT("Grapple")), FText::FromString(TEXT("2")), FLinearColor::White, 1);
	TestEqual("slot carries its icon", Bar->GetSlot(1)->GetIconIndex(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsIconStrokesTest, "Mvs.HUD.IconStrokes", MvsHudTests::Flags)
bool FMvsIconStrokesTest::RunTest(const FString& Parameters)
{
	for (int32 i = 0; i < static_cast<int32>(EMvsGadgetIcon::Count); ++i)
	{
		const TArray<TArray<FVector2f>>& Strokes = MvsGadgetIconStrokes(static_cast<EMvsGadgetIcon>(i));
		TestTrue(*FString::Printf(TEXT("icon %d has strokes"), i), Strokes.Num() > 0);
		for (const TArray<FVector2f>& Line : Strokes)
		{
			TestTrue("every stroke is at least a segment", Line.Num() >= 2);
			for (const FVector2f& P : Line)
			{
				TestTrue("icons stay inside the unit box", FMath::Abs(P.X) <= 1.f && FMath::Abs(P.Y) <= 1.f);
			}
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
