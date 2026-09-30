// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"

/** How a setting is written to the config file. The format is what players' saved settings already use. */
enum class EGothamSettingStorage : uint8
{
	Int,      // the choice index as a number
	Bool,     // "True" / "False"
	Culture,  // the language's culture code
};

/**
 * Everything the settings code needs to know about one option. Each option's value is read as a choice index in
 * [0, ChoiceCount), so stepping, the selector's position pips and the config round trip work the same for all of
 * them; only GetIndex / SetIndex / FormatValue know the actual field.
 */
struct FGothamSettingDescriptor
{
	EGothamSetting Id;
	/** Key in the config section. Never rename one: players' saved settings refer to it. */
	const TCHAR* ConfigKey;
	EGothamSettingStorage Storage;
	FText Label;
	/** One or two sentences for the settings screen's detail pane. */
	FText Description;
	int32 ChoiceCount;
	/** Stepping past the last choice wraps to the first. UI scale clamps instead, and clamps on load too. */
	bool bWraps;
	/** The current choice, or -1 if the value is not one of the choices (a language set by a dev flag). */
	int32 (*GetIndex)(const FGothamSettingsData&);
	void (*SetIndex)(FGothamSettingsData&, int32);
	FText (*FormatValue)(const FGothamSettingsData&);

	/** Index moved by Direction, wrapped or clamped. */
	int32 Stepped(int32 Index, int32 Direction) const;
};

namespace GothamSettingsTable
{
	/** One descriptor per EGothamSetting, in enum order. */
	MVVMSAMPLE_API const TArray<FGothamSettingDescriptor>& Get();
	MVVMSAMPLE_API const FGothamSettingDescriptor& Find(EGothamSetting Setting);
}
