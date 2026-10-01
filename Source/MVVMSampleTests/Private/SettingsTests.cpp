// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Input/MvsBindings.h"
#include "Input/MvsBindingStore.h"
#include "ViewModels/ControlsViewModel.h"
#include "ViewModels/SettingsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsSettingsTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	TArray<FMvsBindingSlot> SampleBindings()
	{
		return {
			{ TEXT("Attack"), MvsBindings::KeyboardSlot, EKeys::LeftMouseButton },
			{ TEXT("Attack"), MvsBindings::GamepadSlot, EKeys::Gamepad_FaceButton_Bottom },
			{ TEXT("Scan"), MvsBindings::KeyboardSlot, EKeys::E },
			{ TEXT("Scan"), MvsBindings::GamepadSlot, EKeys::Gamepad_DPad_Right },
			{ TEXT("Forensic"), MvsBindings::KeyboardSlot, EKeys::V },
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsCycleTest, "Mvs.Settings.Cycle", MvsSettingsTests::Flags)
bool FMvsSettingsCycleTest::RunTest(const FString& Parameters)
{
	FMvsSettingsData Data;
	TestTrue("colour mode changes", Data.Cycle(EMvsSetting::ColorVision, 1));
	TestTrue("first step is protanopia", Data.ColorMode == EMvsColorMode::Protanopia);

	Data.Cycle(EMvsSetting::ColorVision, -1);
	TestTrue("stepping back returns to default", Data.ColorMode == EMvsColorMode::Default);
	Data.Cycle(EMvsSetting::ColorVision, -1);
	TestTrue("stepping back from default wraps to the last preset", Data.ColorMode == EMvsColorMode::Tritanopia);

	Data = FMvsSettingsData();
	TestFalse("UI scale clamps at the smallest step", [&]() { Data.UIScaleIndex = 0; return Data.Cycle(EMvsSetting::UIScale, -1); }());
	Data.UIScaleIndex = FMvsSettingsData::GetUIScaleSteps().Num() - 1;
	TestFalse("UI scale clamps at the largest step", Data.Cycle(EMvsSetting::UIScale, 1));

	Data = FMvsSettingsData();
	TestTrue("toggle turns high contrast on", Data.Cycle(EMvsSetting::HighContrast, 1) && Data.bHighContrast);
	TestFalse("toggling back", Data.Cycle(EMvsSetting::HighContrast, -1) && Data.bHighContrast);

	Data = FMvsSettingsData();
	Data.Cycle(EMvsSetting::Language, 1);
	TestEqual("language moves to the next culture", Data.Language, FString(TEXT("de")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsPersistenceTest, "Mvs.Settings.Persistence", MvsSettingsTests::Flags)
bool FMvsSettingsPersistenceTest::RunTest(const FString& Parameters)
{
	const TCHAR* Section = TEXT("/Script/MVVMSample.MvsSettingsTest");
	FConfigFile File; // in memory, so the test never touches the real settings

	FMvsSettingsData Written;
	Written.Language = TEXT("ja");
	Written.ColorMode = EMvsColorMode::Deuteranopia;
	Written.UIScaleIndex = 3;
	Written.bHighContrast = true;
	Written.bReducedMotion = true;
	Written.WheelMode = EMvsWheelMode::Toggle;
	Written.ScanMode = EMvsScanMode::Tap;
	Written.SubtitleSize = EMvsSubtitleSize::Large;
	Written.bSubtitleBackground = false;
	Written.SaveToConfig(File, Section);

	FMvsSettingsData Read;
	Read.LoadFromConfig(File, Section);
	TestTrue("everything survives a save and load", Read == Written);

	// Garbage in the file must fall back to safe values instead of producing an invalid state.
	File.SetString(Section, TEXT("Language"), TEXT("xx-BOGUS"));
	File.SetInt64(Section, TEXT("ColorMode"), 99);
	File.SetInt64(Section, TEXT("UIScaleIndex"), -4);
	FMvsSettingsData Recovered;
	Recovered.LoadFromConfig(File, Section);
	TestEqual("unknown language falls back to English", Recovered.Language, FString(TEXT("en")));
	TestTrue("out-of-range colour mode falls back", Recovered.ColorMode == EMvsColorMode::Default);
	TestEqual("negative scale index clamps", Recovered.UIScaleIndex, 0);

	FMvsSettingsData Empty;
	Empty.LoadFromConfig(FConfigFile(), Section);
	TestTrue("a missing section gives defaults", Empty == FMvsSettingsData());
	return true;
}

// Preview / apply / revert: edits show immediately, only Apply commits, Revert returns to the committed values.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsMigrationTest, "Mvs.Settings.LegacyMigration", MvsSettingsTests::Flags)
bool FMvsSettingsMigrationTest::RunTest(const FString& Parameters)
{
	// Review 36: settings saved before the prefix changed (Gotham to Mvs) must survive the rename.
	const TCHAR* Legacy = TEXT("/Script/MVVMSample.GothamSettings");
	const TCHAR* Current = UMvsSettingsSubsystem::GetConfigSection();
	FConfigFile File; // in memory, so the test never touches the real settings

	FMvsSettingsData Written;
	Written.Language = TEXT("de");
	Written.UIScaleIndex = 4;
	Written.bHighContrast = true;
	Written.SubtitleSize = EMvsSubtitleSize::Large;
	Written.SaveToConfig(File, Legacy);

	TestTrue("settings under the old section are moved", UMvsSettingsSubsystem::MigrateLegacySettings(File));
	TestNull("the old section is gone", File.FindSection(Legacy));
	FMvsSettingsData Read;
	Read.LoadFromConfig(File, Current);
	TestTrue("every value arrives in the current section", Read == Written);

	// Once moved (or for a new player), nothing happens, and newer values are never overwritten by old ones.
	TestFalse("a second run moves nothing", UMvsSettingsSubsystem::MigrateLegacySettings(File));
	FMvsSettingsData Stale;
	Stale.Language = TEXT("ja");
	Stale.SaveToConfig(File, Legacy);
	TestFalse("old values never overwrite current ones", UMvsSettingsSubsystem::MigrateLegacySettings(File));
	Read.LoadFromConfig(File, Current);
	TestEqual("the current language is kept", Read.Language, FString(TEXT("de")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsViewModelTest, "Mvs.Settings.ViewModel", MvsSettingsTests::Flags)
bool FMvsSettingsViewModelTest::RunTest(const FString& Parameters)
{
	USettingsViewModel* VM = NewObject<USettingsViewModel>(GetTransientPackage());
	VM->Initialize(FMvsSettingsData());

	int32 Previews = 0;
	int32 Commits = 0;
	VM->OnPreview.AddLambda([&Previews](const FMvsSettingsData&) { ++Previews; });
	VM->OnCommitted.AddLambda([&Commits](const FMvsSettingsData&) { ++Commits; });

	TestFalse("starts clean", VM->GetIsDirty());

	VM->Cycle(EMvsSetting::HighContrast, 1);
	TestTrue("edit is dirty", VM->GetIsDirty());
	TestEqual("edit previews immediately", Previews, 1);
	TestEqual("nothing committed yet", Commits, 0);
	TestEqual("value text reflects the edit", VM->GetValueText(EMvsSetting::HighContrast).ToString(), FString(TEXT("On")));

	VM->Revert();
	TestFalse("revert clears dirty", VM->GetIsDirty());
	TestFalse("revert restores the value", VM->GetCurrent().bHighContrast);
	TestEqual("revert previews the restored values", Previews, 2);

	VM->Cycle(EMvsSetting::ReducedMotion, 1);
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsPaletteTest, "Mvs.Accessibility.Palette", MvsSettingsTests::Flags)
bool FMvsPaletteTest::RunTest(const FString& Parameters)
{
	auto Distance = [](const FLinearColor& A, const FLinearColor& B)
	{
		return FVector(A.R - B.R, A.G - B.G, A.B - B.B).Size();
	};

	for (int32 ModeIndex = 0; ModeIndex < static_cast<int32>(EMvsColorMode::Count); ++ModeIndex)
	{
		const EMvsColorMode Mode = static_cast<EMvsColorMode>(ModeIndex);
		for (const bool bHighContrast : { false, true })
		{
			const FLinearColor Good = MvsPalette::Resolve(EMvsColorToken::Good, Mode, bHighContrast);
			const FLinearColor Danger = MvsPalette::Resolve(EMvsColorToken::Danger, Mode, bHighContrast);
			const FLinearColor Unscanned = MvsPalette::Resolve(EMvsColorToken::Unscanned, Mode, bHighContrast);
			const FLinearColor Scanned = MvsPalette::Resolve(EMvsColorToken::Scanned, Mode, bHighContrast);
			const FString Where = FString::Printf(TEXT("mode %d, contrast %d"), ModeIndex, bHighContrast ? 1 : 0);

			TestTrue(*FString::Printf(TEXT("good and danger are far apart (%s)"), *Where), Distance(Good, Danger) > 0.5f);
			TestTrue(*FString::Printf(TEXT("scanned and unscanned are far apart (%s)"), *Where), Distance(Scanned, Unscanned) > 0.5f);
		}
	}

	// Colour-blind presets must not keep the default red/green pairing.
	const FLinearColor DefaultGood = MvsPalette::Resolve(EMvsColorToken::Good, EMvsColorMode::Default, false);
	const FLinearColor DeutGood = MvsPalette::Resolve(EMvsColorToken::Good, EMvsColorMode::Deuteranopia, false);
	TestTrue("deuteranopia recolours 'good' away from green", Distance(DefaultGood, DeutGood) > 0.4f);

	const FLinearColor Base = MvsPalette::Resolve(EMvsColorToken::Info, EMvsColorMode::Default, false);
	const FLinearColor Boosted = MvsPalette::Resolve(EMvsColorToken::Info, EMvsColorMode::Default, true);
	TestTrue("high contrast is at least as bright", FMath::Max3(Boosted.R, Boosted.G, Boosted.B) >= FMath::Max3(Base.R, Base.G, Base.B));
	TestTrue("high contrast makes panels more opaque", MvsPalette::PanelAlpha(true) > MvsPalette::PanelAlpha(false));
	return true;
}

// Rebinding: allowed keys per slot, and conflicts swap instead of leaving a key bound twice.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsRebindTest, "Mvs.Input.Rebinding", MvsSettingsTests::Flags)
bool FMvsRebindTest::RunTest(const FString& Parameters)
{
	using namespace MvsBindings;

	TestTrue("a letter is fine for the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::R));
	TestTrue("mouse buttons are fine for the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::RightMouseButton));
	TestFalse("a gamepad button is not allowed on the keyboard slot", IsKeyAllowedForSlot(KeyboardSlot, EKeys::Gamepad_FaceButton_Top));
	TestFalse("a keyboard key is not allowed on the gamepad slot", IsKeyAllowedForSlot(GamepadSlot, EKeys::R));
	TestTrue("a gamepad button is fine for the gamepad slot", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_FaceButton_Top));
	TestFalse("Escape is reserved for cancel", IsKeyAllowedForSlot(KeyboardSlot, EKeys::Escape));
	TestFalse("gamepad B is reserved for cancel", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_FaceButton_Right));
	TestFalse("axes cannot be bound as buttons", IsKeyAllowedForSlot(GamepadSlot, EKeys::Gamepad_LeftX));
	TestFalse("an invalid key is rejected", IsKeyAllowedForSlot(KeyboardSlot, FKey()));

	const TArray<FMvsBindingSlot> Current = MvsSettingsTests::SampleBindings();

	TArray<FMvsBindingChange> Plain = PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::R);
	TestEqual("an unused key is a single change", Plain.Num(), 1);

	TArray<FMvsBindingChange> Swap = PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::V);
	TestEqual("a used key swaps two slots", Swap.Num(), 2);
	if (Swap.Num() == 2)
	{
		TestTrue("Scan takes V", Swap[0].Name == TEXT("Scan") && Swap[0].NewKey == EKeys::V);
		TestTrue("Forensic receives Scan's old key", Swap[1].Name == TEXT("Forensic") && Swap[1].NewKey == EKeys::E);
	}

	TestTrue("re-assigning the same key is a no-op", PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::E).IsEmpty());
	TestTrue("a disallowed key plans nothing", PlanRebind(Current, TEXT("Scan"), KeyboardSlot, EKeys::Escape).IsEmpty());
	TestTrue("an unknown slot plans nothing", PlanRebind(Current, TEXT("Nope"), KeyboardSlot, EKeys::R).IsEmpty());

	// A gamepad button never conflicts with a keyboard key, even if names matched.
	TArray<FMvsBindingChange> Pad = PlanRebind(Current, TEXT("Scan"), GamepadSlot, EKeys::Gamepad_FaceButton_Bottom);
	TestEqual("gamepad conflict swaps within the gamepad slots", Pad.Num(), 2);

	// The view model validates and applies through its store; here a store in memory.
	struct FMemoryStore final : IMvsBindingStore
	{
		TArray<FMvsBindingSlot> Bindings;
		TArray<FMvsBindingSlot> Defaults;
		int32 Applies = 0;
		virtual TArray<FMvsBindingSlot> GetBindings() const override { return Bindings; }
		virtual void Apply(const TArray<FMvsBindingChange>& Changes) override
		{
			++Applies;
			for (const FMvsBindingChange& Change : Changes)
			{
				for (FMvsBindingSlot& Binding : Bindings)
				{
					Binding.Key = Binding.Name == Change.Name && Binding.Slot == Change.Slot ? Change.NewKey : Binding.Key;
				}
			}
		}
		virtual void ResetToDefaults() override { Bindings = Defaults; }
	};
	const TSharedRef<FMemoryStore> Store = MakeShared<FMemoryStore>();
	Store->Bindings = Current;
	Store->Defaults = Current;
	UControlsViewModel* VM = NewObject<UControlsViewModel>(GetTransientPackage());
	VM->SetStore(Store);
	TestEqual("view model reads the store", VM->GetKey(TEXT("Scan"), KeyboardSlot), EKeys::E);

	TestFalse("view model rejects a keyboard key for a gamepad slot", VM->RequestRebind(TEXT("Scan"), GamepadSlot, EKeys::R));
	TestFalse("and explains why", VM->GetStatusText().IsEmpty());
	TestEqual("and applies nothing", Store->Applies, 0);
	TestTrue("view model accepts a valid rebind", VM->RequestRebind(TEXT("Scan"), KeyboardSlot, EKeys::V));
	TestEqual("applies it once", Store->Applies, 1);
	TestTrue("and shows the swap", VM->GetKey(TEXT("Scan"), KeyboardSlot) == EKeys::V && VM->GetKey(TEXT("Forensic"), KeyboardSlot) == EKeys::E);
	VM->ResetToDefaults();
	TestEqual("reset restores the defaults", VM->GetKey(TEXT("Scan"), KeyboardSlot), EKeys::E);
	TestFalse("and says so", VM->GetStatusText().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSubtitleTest, "Mvs.ViewModels.Subtitles", MvsSettingsTests::Flags)
bool FMvsSubtitleTest::RunTest(const FString& Parameters)
{
	USubtitleViewModel* VM = NewObject<USubtitleViewModel>(GetTransientPackage());
	TestFalse("hidden with no line", VM->GetIsVisible());

	VM->SetLine(FText::FromString(TEXT("Forensic")), FText::FromString(TEXT("Found it.")));
	TestTrue("visible with a line", VM->GetIsVisible());

	VM->Clear();
	TestFalse("hidden again after clearing", VM->GetIsVisible());

	FMvsSettingsData Data;
	Data.SubtitleSize = EMvsSubtitleSize::Large;
	VM->SetPresentation(Data.GetSubtitleFontSize(), false);
	TestTrue("large subtitles use a bigger font", VM->GetFontSize() > FMvsSettingsData().GetSubtitleFontSize());
	TestFalse("background can be turned off", VM->GetHasBackground());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
