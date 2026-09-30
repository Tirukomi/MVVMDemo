// Copyright Epic Games, Inc. All Rights Reserved.

// P4a: the settings descriptor table must agree with the hand-written switches it will replace, for every setting
// and every choice: label, description, value text, selector position, stepping both ways, and the config format.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTable.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamSettingsTableTests
{
	const TCHAR* Section = TEXT("/Script/MVVMSample.GothamSettingsTableTest");

	/** What the descriptor would write for Data, in the config file's text form. */
	FString StoredValue(const FGothamSettingDescriptor& Desc, const FGothamSettingsData& Data)
	{
		switch (Desc.Storage)
		{
		case EGothamSettingStorage::Culture: return Data.Language;
		case EGothamSettingStorage::Bool:    return Desc.GetIndex(Data) != 0 ? TEXT("True") : TEXT("False");
		default:                             return FString::FromInt(Desc.GetIndex(Data));
		}
	}

	/** Every reachable value of one setting (from defaults), plus a language outside the list (the dev flag). */
	TArray<FGothamSettingsData> States(EGothamSetting Setting)
	{
		FGothamSettingsData Data;
		int32 Index = 0, Count = 0;
		Data.GetOptionPosition(Setting, Index, Count);
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
		if (Setting == EGothamSetting::Language)
		{
			FGothamSettingsData Unknown;
			Unknown.Language = TEXT("fr");
			Out.Add(Unknown);
		}
		return Out;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsTableParityTest, "Gotham.Settings.TableMatchesSwitches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamSettingsTableParityTest::RunTest(const FString& Parameters)
{
	using namespace GothamSettingsTableTests;
	TestEqual("one descriptor per setting", GothamSettingsTable::Get().Num(), static_cast<int32>(EGothamSetting::Count));
	USettingsViewModel* ViewModel = NewObject<USettingsViewModel>();

	for (int32 s = 0; s < static_cast<int32>(EGothamSetting::Count); ++s)
	{
		const EGothamSetting Setting = static_cast<EGothamSetting>(s);
		const FGothamSettingDescriptor& Desc = GothamSettingsTable::Find(Setting);
		const FString Name = Desc.ConfigKey;
		TestEqual(Name + TEXT(": label"), Desc.Label.ToString(), USettingsViewModel::GetLabel(Setting).ToString());
		TestEqual(Name + TEXT(": description"), Desc.Description.ToString(), USettingsViewModel::GetDescription(Setting).ToString());

		for (const FGothamSettingsData& Data : States(Setting))
		{
			const int32 Index = Desc.GetIndex(Data);
			const FString At = FString::Printf(TEXT("%s at choice %d"), *Name, Index);

			ViewModel->Initialize(Data);
			TestEqual(At + TEXT(": value text"), Desc.FormatValue(Data).ToString(), ViewModel->GetValueText(Setting).ToString());

			int32 OldIndex = 0, OldCount = 0;
			Data.GetOptionPosition(Setting, OldIndex, OldCount);
			TestEqual(At + TEXT(": choice count"), Desc.ChoiceCount, OldCount);
			TestEqual(At + TEXT(": position"), FMath::Clamp(Index, 0, Desc.ChoiceCount - 1), OldIndex);

			for (const int32 Direction : { -1, +1 })
			{
				FGothamSettingsData Old = Data;
				Old.Cycle(Setting, Direction);
				FGothamSettingsData New = Data;
				Desc.SetIndex(New, Desc.Stepped(FMath::Max(Index, 0), Direction));
				TestTrue(FString::Printf(TEXT("%s: step %+d"), *At, Direction), Old == New);
			}

			FConfigFile File;
			Data.SaveToConfig(File, Section);
			FString Written;
			File.GetString(Section, Desc.ConfigKey, Written);
			TestEqual(At + TEXT(": stored value"), StoredValue(Desc, Data), Written);
		}

		// Out-of-range numbers in a config file: wrapping options fall back to the default, UI scale clamps.
		if (Desc.Storage == EGothamSettingStorage::Int)
		{
			for (const int32 Bad : { -1, 99 })
			{
				FConfigFile File;
				File.SetInt64(Section, Desc.ConfigKey, Bad);
				FGothamSettingsData Loaded;
				Loaded.LoadFromConfig(File, Section);
				const int32 Expected = Desc.bWraps ? Desc.GetIndex(FGothamSettingsData()) : FMath::Clamp(Bad, 0, Desc.ChoiceCount - 1);
				TestEqual(FString::Printf(TEXT("%s: loading %d"), *Name, Bad), Desc.GetIndex(Loaded), Expected);
			}
		}
	}
	return true;
}

#endif
