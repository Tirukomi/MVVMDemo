// Copyright Epic Games, Inc. All Rights Reserved.

// Characterization tests (refactoring pass P0). They pin today's observable behaviour and on-disk formats so the
// refactoring passes (see Docs/RefactoringPlan.md) can change structure without changing results. If one of these
// fails after a refactor, the refactor changed behaviour.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTypes.h"
#include "Core/GothamPlayerController.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Input/GothamBindings.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Friend of AGothamPlayerController: builds its input assets without a world or local player. */
struct FGothamInputTestAccess
{
	static UInputMappingContext* Build(AGothamPlayerController* PC)
	{
		PC->BuildInputAssets();
		return PC->GameplayContext;
	}
};

namespace GothamCharacterizationTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
	const TCHAR* Section = TEXT("/Script/MVVMSample.GothamSettingsTest");

	/** Every reachable value of every setting: start from defaults and cycle forward through all choices. */
	TArray<FGothamSettingsData> AllChoices(EGothamSetting Setting)
	{
		FGothamSettingsData Data;
		int32 Index = 0, Count = 0;
		Data.GetOptionPosition(Setting, Index, Count);
		// Walk to the first choice, then collect each one (UI scale clamps, so step back to its start first).
		for (int32 i = 0; i < Count; ++i)
		{
			Data.Cycle(Setting, -1);
		}
		TArray<FGothamSettingsData> Out;
		for (int32 i = 0; i < Count; ++i)
		{
			Out.Add(Data);
			Data.Cycle(Setting, +1);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsEveryChoiceRoundTripTest, "Gotham.Characterization.SettingsEveryChoiceRoundTrips", GothamCharacterizationTests::Flags)
bool FGothamSettingsEveryChoiceRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace GothamCharacterizationTests;
	for (int32 s = 0; s < static_cast<int32>(EGothamSetting::Count); ++s)
	{
		const EGothamSetting Setting = static_cast<EGothamSetting>(s);
		const TArray<FGothamSettingsData> Choices = AllChoices(Setting);
		TestTrue(FString::Printf(TEXT("setting %d has at least two choices"), s), Choices.Num() >= 2);
		for (int32 c = 0; c < Choices.Num(); ++c)
		{
			FConfigFile File;
			Choices[c].SaveToConfig(File, Section);
			FGothamSettingsData Read;
			Read.LoadFromConfig(File, Section);
			TestTrue(FString::Printf(TEXT("setting %d, choice %d survives save and load"), s, c), Read == Choices[c]);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsConfigFormatTest, "Gotham.Characterization.SettingsConfigFormat", GothamCharacterizationTests::Flags)
bool FGothamSettingsConfigFormatTest::RunTest(const FString& Parameters)
{
	using namespace GothamCharacterizationTests;

	// 1. What SaveToConfig writes today, key by key. Players' saved settings are in this format.
	FGothamSettingsData Data;
	Data.Language = TEXT("de");
	Data.ColorMode = EGothamColorMode::Deuteranopia;
	Data.UIScaleIndex = 3;
	Data.bHighContrast = true;
	Data.bReducedMotion = false;
	Data.WheelMode = EGothamWheelMode::Toggle;
	Data.ScanMode = EGothamScanMode::Tap;
	Data.SubtitleSize = EGothamSubtitleSize::Large;
	Data.bSubtitleBackground = true;
	FConfigFile File;
	Data.SaveToConfig(File, Section);

	const TPair<const TCHAR*, const TCHAR*> Expected[] = {
		{ TEXT("Language"), TEXT("de") },
		{ TEXT("ColorMode"), TEXT("2") },
		{ TEXT("UIScaleIndex"), TEXT("3") },
		{ TEXT("HighContrast"), TEXT("True") },
		{ TEXT("ReducedMotion"), TEXT("False") },
		{ TEXT("WheelMode"), TEXT("1") },
		{ TEXT("ScanMode"), TEXT("1") },
		{ TEXT("SubtitleSize"), TEXT("2") },
		{ TEXT("SubtitleBackground"), TEXT("True") },
	};
	for (const auto& Pair : Expected)
	{
		FString Value;
		TestTrue(FString::Printf(TEXT("writes key %s"), Pair.Key), File.GetString(Section, Pair.Key, Value));
		TestEqual(FString::Printf(TEXT("key %s is written as today"), Pair.Key), Value, FString(Pair.Value));
	}

	// 2. A file in today's exact text format still loads (this is what an existing player's config looks like).
	const FString Fixture = FString::Printf(TEXT(
		"[%s]\n"
		"Language=ja\n"
		"ColorMode=3\n"
		"UIScaleIndex=4\n"
		"HighContrast=True\n"
		"ReducedMotion=True\n"
		"WheelMode=1\n"
		"ScanMode=1\n"
		"SubtitleSize=0\n"
		"SubtitleBackground=False\n"), Section);
	FConfigFile FromText;
	FromText.ProcessInputFileContents(Fixture, TEXT("CharacterizationFixture"));
	FGothamSettingsData Loaded;
	Loaded.LoadFromConfig(FromText, Section);
	TestEqual("fixture: language", Loaded.Language, FString(TEXT("ja")));
	TestTrue("fixture: colour mode", Loaded.ColorMode == EGothamColorMode::Tritanopia);
	TestEqual("fixture: UI scale", Loaded.UIScaleIndex, 4);
	TestTrue("fixture: high contrast", Loaded.bHighContrast);
	TestTrue("fixture: reduced motion", Loaded.bReducedMotion);
	TestTrue("fixture: wheel mode", Loaded.WheelMode == EGothamWheelMode::Toggle);
	TestTrue("fixture: scan mode", Loaded.ScanMode == EGothamScanMode::Tap);
	TestTrue("fixture: subtitle size", Loaded.SubtitleSize == EGothamSubtitleSize::Small);
	TestFalse("fixture: subtitle background", Loaded.bSubtitleBackground);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsTextTest, "Gotham.Characterization.SettingsText", GothamCharacterizationTests::Flags)
bool FGothamSettingsTextTest::RunTest(const FString& Parameters)
{
	using namespace GothamCharacterizationTests;
	for (int32 s = 0; s < static_cast<int32>(EGothamSetting::Count); ++s)
	{
		const EGothamSetting Setting = static_cast<EGothamSetting>(s);
		TestFalse(FString::Printf(TEXT("setting %d has a label"), s), USettingsViewModel::GetLabel(Setting).IsEmpty());
		TestFalse(FString::Printf(TEXT("setting %d has a description"), s), USettingsViewModel::GetDescription(Setting).IsEmpty());

		TSet<FString> Seen;
		for (const FGothamSettingsData& Choice : AllChoices(Setting))
		{
			USettingsViewModel* VM = NewObject<USettingsViewModel>();
			VM->Initialize(Choice);
			const FString Value = VM->GetValueText(Setting).ToString();
			TestFalse(FString::Printf(TEXT("setting %d: every choice has value text"), s), Value.IsEmpty());
			TestFalse(FString::Printf(TEXT("setting %d: value text '%s' is unique among its choices"), s, *Value), Seen.Contains(Value));
			Seen.Add(Value);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamInputBindingsTest, "Gotham.Characterization.InputBindings", GothamCharacterizationTests::Flags)
bool FGothamInputBindingsTest::RunTest(const FString& Parameters)
{
	// Saved rebinds refer to these mappable names, in this order on the Controls screen. Renaming one loses players'
	// rebinds for it, so this list only changes on purpose.
	const TArray<FName> ExpectedNames = {
		TEXT("MoveForward"), TEXT("MoveBack"), TEXT("MoveLeft"), TEXT("MoveRight"), TEXT("Attack"), TEXT("Counter"),
		TEXT("Gadget1"), TEXT("Gadget2"), TEXT("Gadget3"), TEXT("GadgetWheel"), TEXT("Detective"), TEXT("Scan"),
		TEXT("ClueLog"), TEXT("Pause") };
	TArray<FName> Names;
	for (const FGothamBindingDef& Def : GothamBindings::GetDefinitions())
	{
		Names.Add(Def.Name);
	}
	TestEqual("binding list (names and order) is unchanged", Names, ExpectedNames);

	AGothamPlayerController* PC = NewObject<AGothamPlayerController>(GetTransientPackage());
	const UInputMappingContext* Context = FGothamInputTestAccess::Build(PC);
	if (!TestNotNull("the gameplay mapping context is built", Context))
	{
		return false;
	}

	for (const FGothamBindingDef& Def : GothamBindings::GetDefinitions())
	{
		bool bKeyboard = false;
		bool bGamepad = false;
		for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
		{
			if (Mapping.GetMappingName() == Def.Name)
			{
				(Mapping.Key.IsGamepadKey() ? bGamepad : bKeyboard) = true;
			}
		}
		TestTrue(FString::Printf(TEXT("%s has a keyboard / mouse mapping"), *Def.Name.ToString()), bKeyboard);
		TestEqual(FString::Printf(TEXT("%s has a gamepad mapping exactly when it declares a gamepad slot"), *Def.Name.ToString()), bGamepad, Def.bHasGamepadSlot);

		// Everything the UI looks up by name (glyphs, toggles) resolves; the four move directions are keyboard-only
		// actions that nothing looks up.
		if (!Def.Name.ToString().StartsWith(TEXT("Move")))
		{
			TestNotNull(FString::Printf(TEXT("FindAction resolves %s"), *Def.Name.ToString()), PC->FindAction(Def.Name));
		}
	}

	// Today's default keys for the actions the UI names on screen.
	const TPair<FName, FKey> Defaults[] = {
		{ TEXT("Attack"), EKeys::LeftMouseButton }, { TEXT("Counter"), EKeys::RightMouseButton },
		{ TEXT("Scan"), EKeys::E }, { TEXT("ClueLog"), EKeys::J }, { TEXT("Pause"), EKeys::Escape },
		{ TEXT("Counter"), EKeys::Gamepad_RightShoulder }, { TEXT("Pause"), EKeys::Gamepad_Special_Right } };
	for (const auto& Pair : Defaults)
	{
		const bool bFound = Context->GetMappings().ContainsByPredicate([&Pair](const FEnhancedActionKeyMapping& M)
		{
			return M.GetMappingName() == Pair.Key && M.Key == Pair.Value;
		});
		TestTrue(FString::Printf(TEXT("%s defaults to %s"), *Pair.Key.ToString(), *Pair.Value.ToString()), bFound);
	}
	PC->MarkAsGarbage();
	return true;
}

#endif
