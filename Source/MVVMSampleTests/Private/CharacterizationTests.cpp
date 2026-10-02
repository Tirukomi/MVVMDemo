// Copyright IG. All Rights Reserved.

// Characterization tests (refactoring pass P0). They pin today's observable behaviour and on-disk formats so the
// refactoring passes (see Docs/History/RefactoringPlan.md) can change structure without changing results. If one of these
// fails after a refactor, the refactor changed behaviour.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTypes.h"
#include "Core/MvsPlayerController.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Input/MvsBindings.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

/** Friend of AMvsPlayerController: builds its input assets without a world or local player. */
struct FMvsInputTestAccess
{
	static UInputMappingContext* Build(AMvsPlayerController* PC)
	{
		PC->BuildInputAssets();
		return PC->GameplayContext;
	}
};

namespace MvsCharacterizationTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
	const TCHAR* Section = TEXT("/Script/MVVMSample.MvsSettingsTest");

	/** Every reachable value of every setting: start from defaults and cycle forward through all choices. */
	TArray<FMvsSettingsData> AllChoices(EMvsSetting Setting)
	{
		FMvsSettingsData Data;
		int32 Index = 0, Count = 0;
		Data.GetOptionPosition(Setting, Index, Count);
		// Walk to the first choice, then collect each one (UI scale clamps, so step back to its start first).
		for (int32 i = 0; i < Count; ++i)
		{
			Data.Cycle(Setting, -1);
		}
		TArray<FMvsSettingsData> Out;
		for (int32 i = 0; i < Count; ++i)
		{
			Out.Add(Data);
			Data.Cycle(Setting, +1);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsEveryChoiceRoundTripTest, "Mvs.Characterization.SettingsEveryChoiceRoundTrips", MvsCharacterizationTests::Flags)
bool FMvsSettingsEveryChoiceRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace MvsCharacterizationTests;
	for (int32 s = 0; s < static_cast<int32>(EMvsSetting::Count); ++s)
	{
		const EMvsSetting Setting = static_cast<EMvsSetting>(s);
		const TArray<FMvsSettingsData> Choices = AllChoices(Setting);
		TestTrue(FString::Printf(TEXT("setting %d has at least two choices"), s), Choices.Num() >= 2);
		for (int32 c = 0; c < Choices.Num(); ++c)
		{
			FConfigFile File;
			Choices[c].SaveToConfig(File, Section);
			FMvsSettingsData Read;
			Read.LoadFromConfig(File, Section);
			TestTrue(FString::Printf(TEXT("setting %d, choice %d survives save and load"), s, c), Read == Choices[c]);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsConfigFormatTest, "Mvs.Characterization.SettingsConfigFormat", MvsCharacterizationTests::Flags)
bool FMvsSettingsConfigFormatTest::RunTest(const FString& Parameters)
{
	using namespace MvsCharacterizationTests;

	// 1. What SaveToConfig writes today, key by key. Players' saved settings are in this format.
	FMvsSettingsData Data;
	Data.Language = TEXT("de");
	Data.ColorMode = EMvsColorMode::Deuteranopia;
	Data.UIScaleIndex = 3;
	Data.bHighContrast = true;
	Data.bReducedMotion = false;
	Data.WheelMode = EMvsWheelMode::Toggle;
	Data.ScanMode = EMvsScanMode::Tap;
	Data.SubtitleSize = EMvsSubtitleSize::Large;
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
	FMvsSettingsData Loaded;
	Loaded.LoadFromConfig(FromText, Section);
	TestEqual("fixture: language", Loaded.Language, FString(TEXT("ja")));
	TestTrue("fixture: colour mode", Loaded.ColorMode == EMvsColorMode::Tritanopia);
	TestEqual("fixture: UI scale", Loaded.UIScaleIndex, 4);
	TestTrue("fixture: high contrast", Loaded.bHighContrast);
	TestTrue("fixture: reduced motion", Loaded.bReducedMotion);
	TestTrue("fixture: wheel mode", Loaded.WheelMode == EMvsWheelMode::Toggle);
	TestTrue("fixture: scan mode", Loaded.ScanMode == EMvsScanMode::Tap);
	TestTrue("fixture: subtitle size", Loaded.SubtitleSize == EMvsSubtitleSize::Small);
	TestFalse("fixture: subtitle background", Loaded.bSubtitleBackground);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsTextTest, "Mvs.Characterization.SettingsText", MvsCharacterizationTests::Flags)
bool FMvsSettingsTextTest::RunTest(const FString& Parameters)
{
	using namespace MvsCharacterizationTests;
	for (int32 s = 0; s < static_cast<int32>(EMvsSetting::Count); ++s)
	{
		const EMvsSetting Setting = static_cast<EMvsSetting>(s);
		TestFalse(FString::Printf(TEXT("setting %d has a label"), s), USettingsViewModel::GetLabel(Setting).IsEmpty());
		TestFalse(FString::Printf(TEXT("setting %d has a description"), s), USettingsViewModel::GetDescription(Setting).IsEmpty());

		TSet<FString> Seen;
		for (const FMvsSettingsData& Choice : AllChoices(Setting))
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsInputBindingsTest, "Mvs.Characterization.InputBindings", MvsCharacterizationTests::Flags)
bool FMvsInputBindingsTest::RunTest(const FString& Parameters)
{
	// Saved rebinds refer to these mappable names, in this order on the Controls screen. Renaming one loses players'
	// rebinds for it, so this list only changes on purpose.
	const TArray<FName> ExpectedNames = {
		TEXT("MoveForward"), TEXT("MoveBack"), TEXT("MoveLeft"), TEXT("MoveRight"), TEXT("Attack"), TEXT("Counter"),
		TEXT("Gadget1"), TEXT("Gadget2"), TEXT("Gadget3"), TEXT("GadgetWheel"), TEXT("Forensic"), TEXT("Scan"),
		TEXT("ClueLog"), TEXT("Pause") };
	TArray<FName> Names;
	for (const FMvsBindingDef& Def : MvsBindings::GetDefinitions())
	{
		Names.Add(Def.Name);
	}
	TestEqual("binding list (names and order) is unchanged", Names, ExpectedNames);

	AMvsPlayerController* PC = NewObject<AMvsPlayerController>(GetTransientPackage());
	const UInputMappingContext* Context = FMvsInputTestAccess::Build(PC);
	if (!TestNotNull("the gameplay mapping context is built", Context))
	{
		return false;
	}

	for (const FMvsBindingDef& Def : MvsBindings::GetDefinitions())
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

	// Every mapping, grouped by action: value type, then each key in mapping order ("~" = Y negated), with its
	// mappable name and display name. The key order inside an action decides its rebind slots.
	TMap<FString, FString> Actual;
	for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
	{
		FString& Line = Actual.FindOrAdd(GetNameSafe(Mapping.Action));
		if (Line.IsEmpty())
		{
			Line = FString::Printf(TEXT("%d:"), static_cast<int32>(Mapping.Action->ValueType));
		}
		Line += FString::Printf(TEXT(" %s%s"), *Mapping.Key.ToString(), Mapping.Modifiers.IsEmpty() ? TEXT("") : TEXT("~"));
		if (Mapping.IsPlayerMappable())
		{
			Line += FString::Printf(TEXT("[%s|%s]"), *Mapping.GetMappingName().ToString(), *Mapping.GetDisplayName().ToString());
		}
	}
	const TMap<FString, FString> Expected = {
		{ TEXT("IA_Attack"), TEXT("0: LeftMouseButton[Attack|Attack] Gamepad_FaceButton_Bottom[Attack|Attack] F3[Attack|Attack]") },
		{ TEXT("IA_ClueLog"), TEXT("0: J[ClueLog|Case file] Gamepad_Special_Left[ClueLog|Case file]") },
		{ TEXT("IA_Counter"), TEXT("0: RightMouseButton[Counter|Counter] Gamepad_RightShoulder[Counter|Counter]") },
		{ TEXT("IA_DebugDamage"), TEXT("0: F1") },
		{ TEXT("IA_DebugHeal"), TEXT("0: F2") },
		{ TEXT("IA_Forensic"), TEXT("0: V[Forensic|Forensic mode] Gamepad_DPad_Up[Forensic|Forensic mode]") },
		{ TEXT("IA_Gadget1"), TEXT("0: One[Gadget1|Gadget 1] Gamepad_FaceButton_Left[Gadget1|Gadget 1]") },
		{ TEXT("IA_Gadget2"), TEXT("0: Two[Gadget2|Gadget 2] Gamepad_FaceButton_Top[Gadget2|Gadget 2]") },
		{ TEXT("IA_Gadget3"), TEXT("0: Three[Gadget3|Gadget 3] Gamepad_FaceButton_Right[Gadget3|Gadget 3]") },
		{ TEXT("IA_GadgetWheel"), TEXT("0: Q[GadgetWheel|Gadget wheel] Gamepad_LeftShoulder[GadgetWheel|Gadget wheel]") },
		{ TEXT("IA_Look"), TEXT("2: Mouse2D~ Gamepad_Right2D~") },
		{ TEXT("IA_Move"), TEXT("2: Gamepad_Left2D") },
		{ TEXT("IA_MoveBack"), TEXT("0: S[MoveBack|Move back]") },
		{ TEXT("IA_MoveForward"), TEXT("0: W[MoveForward|Move forward]") },
		{ TEXT("IA_MoveLeft"), TEXT("0: A[MoveLeft|Move left]") },
		{ TEXT("IA_MoveRight"), TEXT("0: D[MoveRight|Move right]") },
		{ TEXT("IA_Pause"), TEXT("0: Escape[Pause|Pause] Gamepad_Special_Right[Pause|Pause]") },
		{ TEXT("IA_Scan"), TEXT("0: E[Scan|Scan clue] Gamepad_DPad_Right[Scan|Scan clue]") },
	};
	TestEqual("same set of actions", Actual.Num(), Expected.Num());
	for (const TPair<FString, FString>& Pair : Expected)
	{
		const FString* Found = Actual.Find(Pair.Key);
		TestEqual(FString::Printf(TEXT("%s mappings"), *Pair.Key), Found ? *Found : FString(TEXT("(missing)")), Pair.Value);
	}
	PC->MarkAsGarbage();
	return true;
}

#endif
