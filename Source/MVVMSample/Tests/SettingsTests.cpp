// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTypes.h"
#include "Input/GothamBindings.h"
#include "ViewModels/ControlsViewModel.h"
#include "ViewModels/SettingsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamSettingsTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	TArray<FGothamBindingSlot> SampleBindings()
	{
		return {
			{ TEXT("Attack"), GothamBindings::KeyboardSlot, EKeys::LeftMouseButton },
			{ TEXT("Attack"), GothamBindings::GamepadSlot, EKeys::Gamepad_FaceButton_Bottom },
			{ TEXT("Scan"), GothamBindings::KeyboardSlot, EKeys::E },
			{ TEXT("Scan"), GothamBindings::GamepadSlot, EKeys::Gamepad_DPad_Right },
			{ TEXT("Detective"), GothamBindings::KeyboardSlot, EKeys::V },
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsCycleTest, "Gotham.Settings.Cycle", GothamSettingsTests::Flags)
bool FGothamSettingsCycleTest::RunTest(const FString& Parameters)
{
	FGothamSettingsData Data;
	TestTrue("colour mode changes", Data.Cycle(EGothamSetting::ColorVision, 1));
	TestTrue("first step is protanopia", Data.ColorMode == EGothamColorMode::Protanopia);

	Data.Cycle(EGothamSetting::ColorVision, -1);
	TestTrue("stepping back returns to default", Data.ColorMode == EGothamColorMode::Default);
	Data.Cycle(EGothamSetting::ColorVision, -1);
	TestTrue("stepping back from default wraps to the last preset", Data.ColorMode == EGothamColorMode::Tritanopia);

	Data = FGothamSettingsData();
	TestFalse("UI scale clamps at the smallest step", [&]() { Data.UIScaleIndex = 0; return Data.Cycle(EGothamSetting::UIScale, -1); }());
	Data.UIScaleIndex = FGothamSettingsData::GetUIScaleSteps().Num() - 1;
	TestFalse("UI scale clamps at the largest step", Data.Cycle(EGothamSetting::UIScale, 1));

	Data = FGothamSettingsData();
	TestTrue("toggle turns high contrast on", Data.Cycle(EGothamSetting::HighContrast, 1) && Data.bHighContrast);
	TestFalse("toggling back", Data.Cycle(EGothamSetting::HighContrast, -1) && Data.bHighContrast);

	Data = FGothamSettingsData();
	Data.Cycle(EGothamSetting::Language, 1);
	TestEqual("language moves to the next culture", Data.Language, FString(TEXT("de")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsPersistenceTest, "Gotham.Settings.Persistence", GothamSettingsTests::Flags)
bool FGothamSettingsPersistenceTest::RunTest(const FString& Parameters)
{
	const TCHAR* Section = TEXT("/Script/MVVMSample.GothamSettingsTest");
	FConfigFile File; // in memory, so the test never touches the real settings

	FGothamSettingsData Written;
	Written.Language = TEXT("ja");
	Written.ColorMode = EGothamColorMode::Deuteranopia;
	Written.UIScaleIndex = 3;
	Written.bHighContrast = true;
	Written.bReducedMotion = true;
	Written.WheelMode = EGothamWheelMode::Toggle;
	Written.SubtitleSize = EGothamSubtitleSize::Large;
	Written.bSubtitleBackground = false;
	Written.SaveToConfig(File, Section);

	FGothamSettingsData Read;
	Read.LoadFromConfig(File, Section);
	TestTrue("everything survives a save and load", Read == Written);

	// Garbage in the file must fall back to safe values instead of producing an invalid state.
	File.SetString(Section, TEXT("Language"), TEXT("xx-BOGUS"));
	File.SetInt64(Section, TEXT("ColorMode"), 99);
	File.SetInt64(Section, TEXT("UIScaleIndex"), -4);
	FGothamSettingsData Recovered;
	Recovered.LoadFromConfig(File, Section);
	TestEqual("unknown language falls back to English", Recovered.Language, FString(TEXT("en")));
	TestTrue("out-of-range colour mode falls back", Recovered.ColorMode == EGothamColorMode::Default);
	TestEqual("negative scale index clamps", Recovered.UIScaleIndex, 0);

	FGothamSettingsData Empty;
	Empty.LoadFromConfig(FConfigFile(), Section);
	TestTrue("a missing section gives defaults", Empty == FGothamSettingsData());
	return true;
}

// Preview / apply / revert: edits show immediately, only Apply commits, Revert returns to the committed values.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsViewModelTest, "Gotham.Settings.ViewModel", GothamSettingsTests::Flags)
bool FGothamSettingsViewModelTest::RunTest(const FString& Parameters)
{
	USettingsViewModel* VM = NewObject<USettingsViewModel>(GetTransientPackage());
	VM->Initialize(FGothamSettingsData());

	int32 Previews = 0;
	int32 Commits = 0;
	VM->OnPreview.AddLambda([&Previews](const FGothamSettingsData&) { ++Previews; });
	VM->OnCommitted.AddLambda([&Commits](const FGothamSettingsData&) { ++Commits; });

	TestFalse("starts clean", VM->GetIsDirty());

	VM->Cycle(EGothamSetting::HighContrast, 1);
	TestTrue("edit is dirty", VM->GetIsDirty());
	TestEqual("edit previews immediately", Previews, 1);
	TestEqual("nothing committed yet", Commits, 0);
	TestEqual("value text reflects the edit", VM->GetValueText(EGothamSetting::HighContrast).ToString(), FString(TEXT("On")));

	VM->Revert();
	TestFalse("revert clears dirty", VM->GetIsDirty());
	TestFalse("revert restores the value", VM->GetCurrent().bHighContrast);
	TestEqual("revert previews the restored values", Previews, 2);

	VM->Cycle(EGothamSetting::ReducedMotion, 1);
	VM->Apply();
	TestEqual("apply commits once", Commits, 1);
	TestFalse("apply clears dirty", VM->GetIsDirty());
	VM->Revert();
	TestTrue("revert after apply keeps the applied value", VM->GetCurrent().bReducedMotion);

	VM->ResetDefaults();
	TestFalse("defaults reset the value", VM->GetCurrent().bReducedMotion);
	TestTrue("defaults count as an uncommitted change", VM->GetIsDirty());
	return true;
}

// Every colour token stays distinguishable from its partner in every preset; high contrast never dims a colour.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamPaletteTest, "Gotham.Accessibility.Palette", GothamSettingsTests::Flags)
bool FGothamPaletteTest::RunTest(const FString& Parameters)
{
	auto Distance = [](const FLinearColor& A, const FLinearColor& B)
	{
		return FVector(A.R - B.R, A.G - B.G, A.B - B.B).Size();
	};

	for (int32 ModeIndex = 0; ModeIndex < static_cast<int32>(EGothamColorMode::Count); ++ModeIndex)
	{
		const EGothamColorMode Mode = static_cast<EGothamColorMode>(ModeIndex);
		for (const bool bHighContrast : { false, true })
		{
			const FLinearColor Good = GothamPalette::Resolve(EGothamColorToken::Good, Mode, bHighContrast);
			const FLinearColor Danger = GothamPalette::Resolve(EGothamColorToken::Danger, Mode, bHighContrast);
			const FLinearColor Unscanned = GothamPalette::Resolve(EGothamColorToken::Unscanned, Mode, bHighContrast);
			const FLinearColor Scanned = GothamPalette::Resolve(EGothamColorToken::Scanned, Mode, bHighContrast);
			const FString Where = FString::Printf(TEXT("mode %d, contrast %d"), ModeIndex, bHighContrast ? 1 : 0);

			TestTrue(*FString::Printf(TEXT("good and danger are far apart (%s)"), *Where), Distance(Good, Danger) > 0.5f);
			TestTrue(*FString::Printf(TEXT("scanned and unscanned are far apart (%s)"), *Where), Distance(Scanned, Unscanned) > 0.5f);
		}
	}

	// Colour-blind presets must not keep the default red/green pairing.
	const FLinearColor DefaultGood = GothamPalette::Resolve(EGothamColorToken::Good, EGothamColorMode::Default, false);
	const FLinearColor DeutGood = GothamPalette::Resolve(EGothamColorToken::Good, EGothamColorMode::Deuteranopia, false);
	TestTrue("deuteranopia recolours 'good' away from green", Distance(DefaultGood, DeutGood) > 0.4f);

	const FLinearColor Base = GothamPalette::Resolve(EGothamColorToken::Info, EGothamColorMode::Default, false);
	const FLinearColor Boosted = GothamPalette::Resolve(EGothamColorToken::Info, EGothamColorMode::Default, true);
	TestTrue("high contrast is at least as bright", FMath::Max3(Boosted.R, Boosted.G, Boosted.B) >= FMath::Max3(Base.R, Base.G, Base.B));
	TestTrue("high contrast makes panels more opaque", GothamPalette::PanelAlpha(true) > GothamPalette::PanelAlpha(false));
	return true;
}

// Rebinding: allowed keys per slot, and conflicts swap instead of leaving a key bound twice.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamRebindTest, "Gotham.Input.Rebinding", GothamSettingsTests::Flags)
bool FGothamRebindTest::RunTest(const FString& Parameters)
{
	using namespace GothamBindings;

	TestTrue("a letter is fine for the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::R));
	TestTrue("mouse buttons are fine for the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::RightMouseButton));
	TestFalse("a gamepad button is not allowed on the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::Gamepad_FaceButton_Top));
	TestFalse("a keyboard key is not allowed on the gamepad slot", IsKeyAllowedForSlot(GamepadSlot, EKeys::R));
	TestTrue("a gamepad button is fine for the gamepad slot", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_FaceButton_Top));
	TestFalse("Escape is reserved for cancel", IsKeyAllowedForSlot(KeyboardSlot, EKeys::Escape));
	TestFalse("gamepad B is reserved for cancel", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_FaceButton_Right));
	TestFalse("axes cannot be bound as buttons", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_LeftX));
	TestFalse("an invalid key is rejected", IsKeyAllowedForSlot(KeyboardSlot, FKey()));

	const TArray<FGothamBindingSlot> Current = GothamSettingsTests::SampleBindings();

	TArray<FGothamBindingChange> Plain = PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::R);
	TestEqual("an unused key is a single change", Plain.Num(), 1);

	TArray<FGothamBindingChange> Swap = PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::V);
	TestEqual("a used key swaps two slots", Swap.Num(), 2);
	if (Swap.Num() == 2)
	{
		TestTrue("Scan takes V", Swap[0].Name == TEXT("Scan") && Swap[0].NewKey == EKeys::V);
		TestTrue("Detective receives Scan's old key", Swap[1].Name == TEXT("Detective") && Swap[1].NewKey == EKeys::E);
	}

	TestTrue("re-assigning the same key is a no-op", PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::E).IsEmpty());
	TestTrue("a disallowed key plans nothing", PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::Escape).IsEmpty());
	TestTrue("an unknown slot plans nothing", PlanRebind(Current, TEXT("Nope"), KeyboardSlot, EKeys::R).IsEmpty());

	// A gamepad button never conflicts with a keyboard key, even if names matched.
	TArray<FGothamBindingChange> Pad = PlanRebind(Current, TEXT("Scan"), GamepadSlot, EKeys::Gamepad_FaceButton_Bottom);
	TestEqual("gamepad conflict swaps within the gamepad slots", Pad.Num(), 2);

	// The view model validates, reports, and hands the plan to its owner.
	UControlsViewModel* VM = NewObject<UControlsViewModel>(GetTransientPackage());
	VM->SetSnapshot(Current);
	TArray<FGothamBindingChange> Applied;
	VM->OnChangesPlanned.AddLambda([&Applied](const TArray<FGothamBindingChange>& Changes) { Applied = Changes; });

	TestFalse("view model rejects a keyboard key for a gamepad slot", VM->RequestRebind(TEXT("Scan"), GamepadSlot, EKeys::R));
	TestFalse("and explains why", VM->GetStatusText().IsEmpty());
	TestTrue("view model accepts a valid rebind", VM->RequestRebind(TEXT("Scan"), KeyboardSlot, EKeys::V));
	TestEqual("and reports the swap", Applied.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSubtitleTest, "Gotham.ViewModels.Subtitles", GothamSettingsTests::Flags)
bool FGothamSubtitleTest::RunTest(const FString& Parameters)
{
	USubtitleViewModel* VM = NewObject<USubtitleViewModel>(GetTransientPackage());
	TestFalse("hidden with no line", VM->GetIsVisible());

	VM->SetLine(FText::FromString(TEXT("Detective")), FText::FromString(TEXT("Found it.")));
	TestTrue("visible with a line", VM->GetIsVisible());

	VM->Clear();
	TestFalse("hidden again after clearing", VM->GetIsVisible());

	FGothamSettingsData Data;
	Data.SubtitleSize = EGothamSubtitleSize::Large;
	VM->SetPresentation(Data.GetSubtitleFontSize(), false);
	TestTrue("large subtitles use a bigger font", VM->GetFontSize() > FGothamSettingsData().GetSubtitleFontSize());
	TestFalse("background can be turned off", VM->GetHasBackground());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
