// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTypes.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "CommonInputSettings.h"
#include "ICommonInputModule.h"
#include "Input/MvsUIInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsV4Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	/** Runs the slide at a fixed frame rate for Seconds; returns the number of frames it kept moving. */
	int32 RunSlide(FMvsSlideRect& Slide, float Fps, float Seconds, float* OutMaxX = nullptr)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSlideRectTest, "Mvs.Menus.HighlightSlide", MvsV4Tests::Flags)
bool FMvsSlideRectTest::RunTest(const FString& Parameters)
{
	FMvsSlideRect Slide;
	TestFalse("starts empty", Slide.HasValue());

	Slide.SetTarget(FVector2f(0.f, 100.f), FVector2f(300.f, 40.f), false);
	TestTrue("the first target is taken immediately (no fly-in from the corner)", Slide.HasValue() && !Slide.IsMoving());
	TestEqual("at the first target", Slide.Position, FVector2f(0.f, 100.f));

	Slide.SetTarget(FVector2f(0.f, 200.f), FVector2f(300.f, 40.f), false);
	TestTrue("a new target starts a slide", Slide.IsMoving());
	Slide.Advance(1.f / 60.f);
	TestTrue("first step covers a good part of the way", Slide.Position.Y > 130.f && Slide.Position.Y < 200.f);
	MvsV4Tests::RunSlide(Slide, 60.f, 0.3f);
	TestFalse("settles within a third of a second", Slide.IsMoving());
	TestEqual("exactly on target once settled", Slide.Position, FVector2f(0.f, 200.f));

	// No overshoot: approaching from the left never passes the target.
	FMvsSlideRect Right;
	Right.SetTarget(FVector2f(0.f, 0.f), FVector2f(10.f, 10.f), false);
	Right.SetTarget(FVector2f(500.f, 0.f), FVector2f(10.f, 10.f), false);
	float MaxX = 0.f;
	MvsV4Tests::RunSlide(Right, 144.f, 0.5f, &MaxX);
	TestTrue("never overshoots", MaxX <= 500.f);

	// Frame-rate independence: after 100 ms, 30 fps and 240 fps land in nearly the same place.
	FMvsSlideRect Slow, Fast;
	for (FMvsSlideRect* S : { &Slow, &Fast })
	{
		S->SetTarget(FVector2f::ZeroVector, FVector2f(10.f, 10.f), false);
		S->SetTarget(FVector2f(0.f, 400.f), FVector2f(10.f, 10.f), false);
	}
	MvsV4Tests::RunSlide(Slow, 30.f, 0.1f);
	MvsV4Tests::RunSlide(Fast, 240.f, 0.1f);
	TestNearlyEqual("frame-rate independent", Slow.Position.Y, Fast.Position.Y, 1.f);

	// Snap (reduced motion) jumps straight there.
	Slide.SetTarget(FVector2f(0.f, 20.f), FVector2f(300.f, 60.f), true);
	TestTrue("snap does not animate", !Slide.IsMoving() && Slide.Position == FVector2f(0.f, 20.f) && Slide.Size == FVector2f(300.f, 60.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsTabsTest, "Mvs.Menus.SettingsTabs", MvsV4Tests::Flags)
bool FMvsSettingsTabsTest::RunTest(const FString& Parameters)
{
	TArray<int32> Seen;
	Seen.SetNumZeroed(static_cast<int32>(EMvsSetting::Count));
	TSet<FName> Ids;
	for (const FMvsSettingsTab& Tab : USettingsViewModel::GetTabs())
	{
		TestFalse(FString::Printf(TEXT("tab %s has a label"), *Tab.Id.ToString()), Tab.Label.IsEmpty());
		TestFalse(FString::Printf(TEXT("tab %s is not empty"), *Tab.Id.ToString()), Tab.Settings.IsEmpty());
		TestFalse(FString::Printf(TEXT("tab id %s is unique"), *Tab.Id.ToString()), Ids.Contains(Tab.Id));
		Ids.Add(Tab.Id);
		for (const EMvsSetting Setting : Tab.Settings)
		{
			++Seen[static_cast<int32>(Setting)];
		}
	}
	for (int32 i = 0; i < Seen.Num(); ++i)
	{
		const EMvsSetting Setting = static_cast<EMvsSetting>(i);
		TestEqual(FString::Printf(TEXT("setting %d is in exactly one tab"), i), Seen[i], 1);
		TestFalse(FString::Printf(TEXT("setting %d has a description"), i), USettingsViewModel::GetDescription(Setting).IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsOptionPositionTest, "Mvs.Menus.OptionPosition", MvsV4Tests::Flags)
bool FMvsOptionPositionTest::RunTest(const FString& Parameters)
{
	for (int32 i = 0; i < static_cast<int32>(EMvsSetting::Count); ++i)
	{
		const EMvsSetting Setting = static_cast<EMvsSetting>(i);
		FMvsSettingsData Data;
		int32 Index = -1, Count = 0;
		Data.GetOptionPosition(Setting, Index, Count);
		TestTrue(FString::Printf(TEXT("setting %d: index within count"), i), Count >= 2 && Index >= 0 && Index < Count);

		// Stepping forward moves the pip forward by one (or wraps / clamps at the end).
		const int32 Before = Index;
		Data.Cycle(Setting, +1);
		Data.GetOptionPosition(Setting, Index, Count);
		const bool bClamps = Setting == EMvsSetting::UIScale;
		const int32 Expected = bClamps ? FMath::Min(Before + 1, Count - 1) : (Before + 1) % Count;
		TestEqual(FString::Printf(TEXT("setting %d: +1 moves the pip"), i), Index, Expected);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsMenuTextTest, "Mvs.Menus.PromptsAndCaseNumbers", MvsV4Tests::Flags)
bool FMvsMenuTextTest::RunTest(const FString& Parameters)
{
	// The menu keys come from one mapping context; Common UI matches keys to actions through it.
	const UMvsUIInputData& Menu = UMvsUIInputData::Get();
	const UInputMappingContext* Context = Menu.BuildMappingContext(GetTransientPackage());
	auto Maps = [Context](const UInputAction* Action, const FKey& Key)
	{
		return Context->GetMappings().ContainsByPredicate([Action, Key](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Action == Action && Mapping.Key == Key; });
	};
	TestTrue("Q steps back", Maps(Menu.GetPreviousTabAction(), EKeys::Q));
	TestTrue("RB steps forward", Maps(Menu.GetNextTabAction(), EKeys::Gamepad_RightShoulder));
	TestFalse("Enter is not a tab key", Maps(Menu.GetNextTabAction(), EKeys::Enter) || Maps(Menu.GetPreviousTabAction(), EKeys::Enter));
	TestTrue("Enter and the platform's accept button accept", Maps(Menu.GetAcceptAction(), EKeys::Enter) && Maps(Menu.GetAcceptAction(), EKeys::Virtual_Gamepad_Accept.GetVirtualKey()));
	TestTrue("Esc and the platform's back button go back", Maps(Menu.GetBackAction(), EKeys::Escape) && Maps(Menu.GetBackAction(), EKeys::Virtual_Gamepad_Back.GetVirtualKey()));
	UCommonInputSettings& CommonInput = ICommonInputModule::GetSettings();
	CommonInput.LoadData();
	TestTrue("Common UI's Enhanced Input support is on", CommonInput.IsEnhancedInputSupportEnabled());
	TestTrue("Common UI uses these actions for accept and back", CommonInput.GetEnhancedInputClickAction() == Menu.GetAcceptAction()
		&& CommonInput.GetEnhancedInputBackAction() == Menu.GetBackAction());
	// Gameplay keys (Q opens the wheel, E scans) share these keys; a consuming menu action would hide them.
	TestFalse("menu actions never consume their keys", Menu.GetAcceptAction()->bConsumeInput || Menu.GetBackAction()->bConsumeInput
		|| Menu.GetPreviousTabAction()->bConsumeInput || Menu.GetNextTabAction()->bConsumeInput);
	TestEqual("shoulder glyphs are abbreviated", UMvsInputGlyph::GetKeyLabel(EKeys::Gamepad_LeftShoulder).ToString(), FString(TEXT("LB")));

	TestEqual("case numbers are 1-based and padded", MvsCaseNumber(6).ToString(), FString(TEXT("No. 007")));
	TestEqual("no grouping in large numbers", MvsCaseNumber(1233).ToString(), FString(TEXT("No. 1234")));
	return true;
}

#endif
