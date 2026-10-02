// Copyright IG. All Rights Reserved.

// Settings edge cases, with literal expectations. The everyday behaviour (every choice's
// text, round trips, the on-disk format) is pinned by Mvs.Characterization.*.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTable.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingRowViewModel.h"
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

// Second review 14: one view model per option. A change notifies only the row it touched, and only the fields that read
// differently; a language refresh re-texts every row.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSettingsRowsTest, "Mvs.Settings.Rows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsSettingsRowsTest::RunTest(const FString& Parameters)
{
	using FRowVM = USettingRowViewModel::FFieldNotificationClassDescriptor;
	USettingsViewModel* ViewModel = NewObject<USettingsViewModel>();
	ViewModel->Initialize(FMvsSettingsData());
	for (int32 i = 0; i < static_cast<int32>(EMvsSetting::Count); ++i)
	{
		const USettingRowViewModel* Row = ViewModel->GetRow(static_cast<EMvsSetting>(i));
		TestTrue(FString::Printf(TEXT("setting %d has a row for itself"), i), Row && Row->GetSetting() == static_cast<EMvsSetting>(i));
	}
	USettingRowViewModel* Contrast = ViewModel->GetRow(EMvsSetting::HighContrast);
	USettingRowViewModel* Scale = ViewModel->GetRow(EMvsSetting::UIScale);
	if (!Contrast || !Scale)
	{
		return false;
	}

	TMap<FName, int32> Notifies;
	auto Count = [&Notifies](USettingRowViewModel* Row, const TCHAR* Prefix)
	{
		for (const UE::FieldNotification::FFieldId Field : { FRowVM::Label, FRowVM::ValueText, FRowVM::Description, FRowVM::ChoiceIndex, FRowVM::ChoiceCount })
		{
			const FName Key(*FString::Printf(TEXT("%s.%s"), Prefix, *Field.GetName().ToString()));
			Row->AddFieldValueChangedDelegate(Field, INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda(
				[&Notifies, Key](UObject*, UE::FieldNotification::FFieldId) { ++Notifies.FindOrAdd(Key); }));
		}
	};
	Count(Contrast, TEXT("Contrast"));
	Count(Scale, TEXT("Scale"));
	auto Total = [&Notifies](const TCHAR* Prefix)
	{
		int32 Sum = 0;
		for (const TPair<FName, int32>& Pair : Notifies)
		{
			Sum += Pair.Key.ToString().StartsWith(Prefix) ? Pair.Value : 0;
		}
		return Sum;
	};

	ViewModel->Cycle(EMvsSetting::HighContrast, +1);
	TestEqual("stepping high contrast changes its value text", Notifies.FindRef(TEXT("Contrast.ValueText")), 1);
	TestEqual("and its position", Notifies.FindRef(TEXT("Contrast.ChoiceIndex")), 1);
	TestEqual("but not its label or description", Notifies.FindRef(TEXT("Contrast.Label")) + Notifies.FindRef(TEXT("Contrast.Description")), 0);
	TestEqual("and no other row", Total(TEXT("Scale.")), 0);
	TestEqual("the row reads the new value", Contrast->GetValueText().ToString(), FString(TEXT("On")));

	Notifies.Reset();
	ViewModel->Apply();
	TestEqual("applying changes nothing on screen", Total(TEXT("")), 0);
	ViewModel->Revert();
	TestEqual("reverting to identical values changes nothing", Total(TEXT("")), 0);

	FMvsSettingsData AtSmallest;
	AtSmallest.UIScaleIndex = 0;
	ViewModel->Initialize(AtSmallest);
	TestFalse("UI scale stops at its ends", Scale->GetWraps());
	TestTrue("other options wrap", Contrast->GetWraps());
	Notifies.Reset();
	Scale->Step(-1);
	TestEqual("a clamped step that changes nothing notifies nothing", Total(TEXT("")), 0);
	Scale->Step(+1);
	TestEqual("a row's step edits its own option", ViewModel->GetCurrent().UIScaleIndex, 1);

	Notifies.Reset();
	ViewModel->RefreshTexts();
	TestEqual("a language refresh re-texts every row's label", Notifies.FindRef(TEXT("Contrast.Label")) + Notifies.FindRef(TEXT("Scale.Label")), 2);
	TestEqual("and values and descriptions", Notifies.FindRef(TEXT("Scale.ValueText")) + Notifies.FindRef(TEXT("Scale.Description")), 2);
	return true;
}

#endif
