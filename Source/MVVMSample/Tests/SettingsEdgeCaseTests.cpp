// Copyright Epic Games, Inc. All Rights Reserved.

// Settings edge cases, with literal expectations. The everyday behaviour (every choice's
// text, round trips, the on-disk format) is pinned by Gotham.Characterization.*.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTable.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsEdgeCasesTest, "Gotham.Settings.EdgeCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamSettingsEdgeCasesTest::RunTest(const FString& Parameters)
{
	const TCHAR* Section = TEXT("/Script/MVVMSample.GothamSettingsTableTest");

	// One descriptor per setting, in enum order (Find indexes by the enum).
	const TArray<FGothamSettingDescriptor>& Table = GothamSettingsTable::Get();
	TestEqual("one descriptor per setting", Table.Num(), static_cast<int32>(EGothamSetting::Count));
	for (int32 i = 0; i < Table.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("descriptor %d is in enum order"), i), Table[i].Id == static_cast<EGothamSetting>(i));
	}

	// UI scale clamps at both ends; everything else wraps.
	{
		FGothamSettingsData Data;
		Data.UIScaleIndex = 0;
		TestFalse("UI scale: stepping below the smallest does nothing", Data.Cycle(EGothamSetting::UIScale, -1));
		Data.UIScaleIndex = FGothamSettingsData::GetUIScaleSteps().Num() - 1;
		TestFalse("UI scale: stepping above the largest does nothing", Data.Cycle(EGothamSetting::UIScale, +1));
		Data.ColorMode = EGothamColorMode::Tritanopia;
		Data.Cycle(EGothamSetting::ColorVision, +1);
		TestTrue("colour vision wraps to the first preset", Data.ColorMode == EGothamColorMode::Default);
	}

	// Booleans toggle in either direction.
	{
		FGothamSettingsData Data;
		Data.Cycle(EGothamSetting::HighContrast, -1);
		TestTrue("a bool steps on with -1", Data.bHighContrast);
		Data.Cycle(EGothamSetting::HighContrast, -1);
		TestFalse("and off again", Data.bHighContrast);
	}

	// A language outside the list (the -GothamLanguage dev flag): shown as its culture code, stepped from the first.
	{
		FGothamSettingsData Data;
		Data.Language = TEXT("fr");
		USettingsViewModel* ViewModel = NewObject<USettingsViewModel>();
		ViewModel->Initialize(Data);
		TestEqual("unknown language shows its code", ViewModel->GetValueText(EGothamSetting::Language).ToString(), FString(TEXT("fr")));
		int32 Index = -1, Count = 0;
		Data.GetOptionPosition(EGothamSetting::Language, Index, Count);
		TestEqual("unknown language sits at the first pip", Index, 0);
		Data.Cycle(EGothamSetting::Language, +1);
		TestEqual("and steps from the first language", Data.Language, FString(FGothamSettingsData::GetLanguages()[1].Culture));
	}

	// Out-of-range numbers and unknown strings in a config file.
	{
		FConfigFile File;
		File.SetInt64(Section, TEXT("ColorMode"), 99);
		File.SetInt64(Section, TEXT("UIScaleIndex"), 99);
		File.SetInt64(Section, TEXT("SubtitleSize"), -1);
		File.SetString(Section, TEXT("Language"), TEXT("xx"));
		FGothamSettingsData Loaded;
		Loaded.LoadFromConfig(File, Section);
		TestTrue("colour mode 99 falls back to the default", Loaded.ColorMode == EGothamColorMode::Default);
		TestEqual("UI scale 99 clamps to the largest step", Loaded.UIScaleIndex, FGothamSettingsData::GetUIScaleSteps().Num() - 1);
		TestTrue("subtitle size -1 falls back to the default", Loaded.SubtitleSize == EGothamSubtitleSize::Medium);
		TestEqual("an unknown language falls back to English", Loaded.Language, FString(TEXT("en")));

		FConfigFile Low;
		Low.SetInt64(Section, TEXT("UIScaleIndex"), -5);
		Loaded.LoadFromConfig(Low, Section);
		TestEqual("UI scale -5 clamps to the smallest step", Loaded.UIScaleIndex, 0);
	}

	// Out-of-range settings ids are harmless.
	{
		FGothamSettingsData Data;
		TestFalse("cycling Count changes nothing", Data.Cycle(EGothamSetting::Count, +1));
		TestTrue("and has no label", USettingsViewModel::GetLabel(EGothamSetting::Count).IsEmpty());
	}
	return true;
}

#endif
