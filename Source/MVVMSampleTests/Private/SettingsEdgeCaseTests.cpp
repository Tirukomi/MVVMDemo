// Copyright IG. All Rights Reserved.

// Settings edge cases, with literal expectations. The everyday behaviour (every choice's
// text, round trips, the on-disk format) is pinned by Mvs.Characterization.*.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTable.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsEdgeCasesTest, "Mvs.Settings.EdgeCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsSettingsEdgeCasesTest::RunTest(const FString& Parameters)
{
	const TCHAR* Section = TEXT("/Script/MVVMSample.MvsSettingsTableTest");

	// One descriptor per setting, in enum order (Find indexes by the enum).
	const TArray<FMvsSettingDescriptor>& Table = MvsSettingsTable::Get();
	TestEqual("one descriptor per setting", Table.Num(), static_cast<int32>(EMvsSetting::Count));
	for (int32 i = 0; i < Table.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("descriptor %d is in enum order"), i), Table[i].Id == static_cast<EMvsSetting>(i));
	}

	// UI scale clamps at both ends; everything else wraps.
	{
		FMvsSettingsData Data;
		Data.UIScaleIndex = 0;
		TestFalse("UI scale: stepping below the smallest does nothing", Data.Cycle(EMvsSetting::UIScale, -1));
		Data.UIScaleIndex = FMvsSettingsData::GetUIScaleSteps().Num() - 1;
		TestFalse("UI scale: stepping above the largest does nothing", Data.Cycle(EMvsSetting::UIScale, +1));
		Data.ColorMode = EMvsColorMode::Tritanopia;
		Data.Cycle(EMvsSetting::ColorVision, +1);
		TestTrue("colour vision wraps to the first preset", Data.ColorMode == EMvsColorMode::Default);
	}

	// Booleans toggle in either direction.
	{
		FMvsSettingsData Data;
		Data.Cycle(EMvsSetting::HighContrast, -1);
		TestTrue("a bool steps on with -1", Data.bHighContrast);
		Data.Cycle(EMvsSetting::HighContrast, -1);
		TestFalse("and off again", Data.bHighContrast);
	}

	// A language outside the list (the -MvsLanguage dev flag): shown as its culture code, stepped from the first.
	{
		FMvsSettingsData Data;
		Data.Language = TEXT("fr");
		USettingsViewModel* ViewModel = NewObject<USettingsViewModel>();
		ViewModel->Initialize(Data);
		TestEqual("unknown language shows its code", ViewModel->GetValueText(EMvsSetting::Language).ToString(), FString(TEXT("fr")));
		int32 Index = -1, Count = 0;
		Data.GetOptionPosition(EMvsSetting::Language, Index, Count);
		TestEqual("unknown language sits at the first pip", Index, 0);
		Data.Cycle(EMvsSetting::Language, +1);
		TestEqual("and steps from the first language", Data.Language, FString(FMvsSettingsData::GetLanguages()[1].Culture));
	}

	// Out-of-range numbers and unknown strings in a config file.
	{
		FConfigFile File;
		File.SetInt64(Section, TEXT("ColorMode"), 99);
		File.SetInt64(Section, TEXT("UIScaleIndex"), 99);
		File.SetInt64(Section, TEXT("SubtitleSize"), -1);
		File.SetString(Section, TEXT("Language"), TEXT("xx"));
		FMvsSettingsData Loaded;
		Loaded.LoadFromConfig(File, Section);
		TestTrue("colour mode 99 falls back to the default", Loaded.ColorMode == EMvsColorMode::Default);
		TestEqual("UI scale 99 clamps to the largest step", Loaded.UIScaleIndex, FMvsSettingsData::GetUIScaleSteps().Num() - 1);
		TestTrue("subtitle size -1 falls back to the default", Loaded.SubtitleSize == EMvsSubtitleSize::Medium);
		TestEqual("an unknown language falls back to English", Loaded.Language, FString(TEXT("en")));

		FConfigFile Low;
		Low.SetInt64(Section, TEXT("UIScaleIndex"), -5);
		Loaded.LoadFromConfig(Low, Section);
		TestEqual("UI scale -5 clamps to the smallest step", Loaded.UIScaleIndex, 0);
	}

	// Out-of-range settings ids are harmless.
	{
		FMvsSettingsData Data;
		TestFalse("cycling Count changes nothing", Data.Cycle(EMvsSetting::Count, +1));
		TestTrue("and has no label", USettingsViewModel::GetLabel(EMvsSetting::Count).IsEmpty());
	}
	return true;
}

// Views refresh on one field, Revision: it must bump whenever a displayed value can have changed, and only then.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsRevisionTest, "Mvs.Settings.Revision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsSettingsRevisionTest::RunTest(const FString& Parameters)
{
	USettingsViewModel* ViewModel = NewObject<USettingsViewModel>();
	ViewModel->Initialize(FMvsSettingsData());
	int32 Notifies = 0;
	ViewModel->AddFieldValueChangedDelegate(USettingsViewModel::FFieldNotificationClassDescriptor::Revision,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&Notifies](UObject*, UE::FieldNotification::FFieldId) { ++Notifies; }));
	const int32 Start = ViewModel->GetRevision();

	ViewModel->Cycle(EMvsSetting::HighContrast, +1);
	TestEqual("a value change bumps", ViewModel->GetRevision(), Start + 1);
	ViewModel->Apply();
	TestEqual("applying changes nothing on screen", ViewModel->GetRevision(), Start + 1);
	ViewModel->Revert();
	TestEqual("reverting to identical values changes nothing", ViewModel->GetRevision(), Start + 1);

	FMvsSettingsData AtSmallest;
	AtSmallest.UIScaleIndex = 0;
	ViewModel->Initialize(AtSmallest);
	const int32 BeforeClamp = ViewModel->GetRevision();
	ViewModel->Cycle(EMvsSetting::UIScale, -1);
	TestEqual("a clamped step that changes nothing does not bump", ViewModel->GetRevision(), BeforeClamp);

	ViewModel->RefreshTexts();
	TestEqual("a language refresh always bumps", ViewModel->GetRevision(), BeforeClamp + 1);
	TestEqual("every bump notifies", Notifies, ViewModel->GetRevision() - Start);
	return true;
}

#endif
