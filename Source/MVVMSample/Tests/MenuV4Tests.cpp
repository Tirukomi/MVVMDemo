// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTypes.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "UI/Widgets/GothamTabList.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamV4Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	/** Runs the slide at a fixed frame rate for Seconds; returns the number of frames it kept moving. */
	int32 RunSlide(FGothamSlideRect& Slide, float Fps, float Seconds, float* OutMaxX = nullptr)
	{
		int32 Moving = 0;
		const int32 Frames = FMath::CeilToInt(Fps * Seconds);
		for (int32 i = 0; i < Frames; ++i)
		{
			Moving += Slide.Advance(1.f / Fps) ? 1 : 0;
			if (OutMaxX) { *OutMaxX = FMath::Max(*OutMaxX, Slide.Position.X); }
		}
		return Moving;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSlideRectTest, "Gotham.Menus.HighlightSlide", GothamV4Tests::Flags)
bool FGothamSlideRectTest::RunTest(const FString& Parameters)
{
	FGothamSlideRect Slide;
	TestFalse("starts empty", Slide.HasValue());

	Slide.SetTarget(FVector2f(0.f, 100.f), FVector2f(300.f, 40.f), false);
	TestTrue("the first target is taken immediately (no fly-in from the corner)", Slide.HasValue() && !Slide.IsMoving());
	TestEqual("at the first target", Slide.Position, FVector2f(0.f, 100.f));

	Slide.SetTarget(FVector2f(0.f, 200.f), FVector2f(300.f, 40.f), false);
	TestTrue("a new target starts a slide", Slide.IsMoving());
	Slide.Advance(1.f / 60.f);
	TestTrue("first step covers a good part of the way", Slide.Position.Y > 130.f && Slide.Position.Y < 200.f);
	GothamV4Tests::RunSlide(Slide, 60.f, 0.3f);
	TestFalse("settles within a third of a second", Slide.IsMoving());
	TestEqual("exactly on target once settled", Slide.Position, FVector2f(0.f, 200.f));

	// No overshoot: approaching from the left never passes the target.
	FGothamSlideRect Right;
	Right.SetTarget(FVector2f(0.f, 0.f), FVector2f(10.f, 10.f), false);
	Right.SetTarget(FVector2f(500.f, 0.f), FVector2f(10.f, 10.f), false);
	float MaxX = 0.f;
	GothamV4Tests::RunSlide(Right, 144.f, 0.5f, &MaxX);
	TestTrue("never overshoots", MaxX <= 500.f);

	// Frame-rate independence: after 100 ms, 30 fps and 240 fps land in nearly the same place.
	FGothamSlideRect Slow, Fast;
	for (FGothamSlideRect* S : { &Slow, &Fast })
	{
		S->SetTarget(FVector2f::ZeroVector, FVector2f(10.f, 10.f), false);
		S->SetTarget(FVector2f(0.f, 400.f), FVector2f(10.f, 10.f), false);
	}
	GothamV4Tests::RunSlide(Slow, 30.f, 0.1f);
	GothamV4Tests::RunSlide(Fast, 240.f, 0.1f);
	TestNearlyEqual("frame-rate independent", Slow.Position.Y, Fast.Position.Y, 1.f);

	// Snap (reduced motion) jumps straight there.
	Slide.SetTarget(FVector2f(0.f, 20.f), FVector2f(300.f, 60.f), true);
	TestTrue("snap does not animate", !Slide.IsMoving() && Slide.Position == FVector2f(0.f, 20.f) && Slide.Size == FVector2f(300.f, 60.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsTabsTest, "Gotham.Menus.SettingsTabs", GothamV4Tests::Flags)
bool FGothamSettingsTabsTest::RunTest(const FString& Parameters)
{
	TArray<int32> Seen;
	Seen.SetNumZeroed(static_cast<int32>(EGothamSetting::Count));
	TSet<FName> Ids;
	for (const FGothamSettingsTab& Tab : USettingsViewModel::GetTabs())
	{
		TestFalse(FString::Printf(TEXT("tab %s has a label"), *Tab.Id.ToString()), Tab.Label.IsEmpty());
		TestFalse(FString::Printf(TEXT("tab %s is not empty"), *Tab.Id.ToString()), Tab.Settings.IsEmpty());
		TestFalse(FString::Printf(TEXT("tab id %s is unique"), *Tab.Id.ToString()), Ids.Contains(Tab.Id));
		Ids.Add(Tab.Id);
		for (const EGothamSetting Setting : Tab.Settings)
		{
			++Seen[static_cast<int32>(Setting)];
		}
	}
	for (int32 i = 0; i < Seen.Num(); ++i)
	{
		const EGothamSetting Setting = static_cast<EGothamSetting>(i);
		TestEqual(FString::Printf(TEXT("setting %d is in exactly one tab"), i), Seen[i], 1);
		TestFalse(FString::Printf(TEXT("setting %d has a description"), i), USettingsViewModel::GetDescription(Setting).IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamOptionPositionTest, "Gotham.Menus.OptionPosition", GothamV4Tests::Flags)
bool FGothamOptionPositionTest::RunTest(const FString& Parameters)
{
	for (int32 i = 0; i < static_cast<int32>(EGothamSetting::Count); ++i)
	{
		const EGothamSetting Setting = static_cast<EGothamSetting>(i);
		FGothamSettingsData Data;
		int32 Index = -1, Count = 0;
		Data.GetOptionPosition(Setting, Index, Count);
		TestTrue(FString::Printf(TEXT("setting %d: index within count"), i), Count >= 2 && Index >= 0 && Index < Count);

		// Stepping forward moves the pip forward by one (or wraps / clamps at the end).
		const int32 Before = Index;
		Data.Cycle(Setting, +1);
		Data.GetOptionPosition(Setting, Index, Count);
		const bool bClamps = Setting == EGothamSetting::UIScale;
		const int32 Expected = bClamps ? FMath::Min(Before + 1, Count - 1) : (Before + 1) % Count;
		TestEqual(FString::Printf(TEXT("setting %d: +1 moves the pip"), i), Index, Expected);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamMenuTextTest, "Gotham.Menus.PromptsAndCaseNumbers", GothamV4Tests::Flags)
bool FGothamMenuTextTest::RunTest(const FString& Parameters)
{
	int32 Direction = 0;
	TestTrue("Q steps back", UGothamTabList::IsTabKey(EKeys::Q, Direction) && Direction == -1);
	TestTrue("RB steps forward", UGothamTabList::IsTabKey(EKeys::Gamepad_RightShoulder, Direction) && Direction == +1);
	TestFalse("Enter is not a tab key", UGothamTabList::IsTabKey(EKeys::Enter, Direction));
	TestEqual("shoulder glyphs are abbreviated", UGothamInputGlyph::GetKeyLabel(EKeys::Gamepad_LeftShoulder).ToString(), FString(TEXT("LB")));

	TestEqual("case numbers are 1-based and padded", GothamCaseNumber(6).ToString(), FString(TEXT("No. 007")));
	TestEqual("no grouping in large numbers", GothamCaseNumber(1233).ToString(), FString(TEXT("No. 1234")));
	return true;
}

#endif
