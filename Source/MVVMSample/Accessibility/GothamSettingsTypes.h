// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"

/** Colour-vision presets. Each maps the semantic colour tokens to a palette that stays distinguishable. */
enum class EGothamColorMode : uint8
{
	Default,
	Protanopia,
	Deuteranopia,
	Tritanopia,
	Count
};

enum class EGothamSubtitleSize : uint8
{
	Small,
	Medium,
	Large,
	Count
};

enum class EGothamWheelMode : uint8
{
	Hold,
	Toggle,
	Count
};

/** Every option the settings screen edits. */
enum class EGothamSetting : uint8
{
	Language,
	ColorVision,
	UIScale,
	HighContrast,
	ReducedMotion,
	WheelMode,
	SubtitleSize,
	SubtitleBackground,
	Count
};

/** A language the player can pick. Names are in their own language on purpose, so they are never translated. */
struct FGothamLanguageOption
{
	const TCHAR* Culture;
	const TCHAR* NativeName;
};

/** The plain data behind the settings screen. No UObjects, so rules and persistence are unit-testable. */
struct MVVMSAMPLE_API FGothamSettingsData
{
	FString Language = TEXT("en");
	EGothamColorMode ColorMode = EGothamColorMode::Default;
	int32 UIScaleIndex = 1;
	bool bHighContrast = false;
	bool bReducedMotion = false;
	EGothamWheelMode WheelMode = EGothamWheelMode::Hold;
	EGothamSubtitleSize SubtitleSize = EGothamSubtitleSize::Medium;
	bool bSubtitleBackground = true;

	static const TArray<float>& GetUIScaleSteps();
	static const TArray<FGothamLanguageOption>& GetLanguages();

	float GetUIScale() const;
	int32 GetSubtitleFontSize() const;

	/** Steps a setting by Direction (+1 / -1), wrapping. Returns true if the value changed. */
	bool Cycle(EGothamSetting Setting, int32 Direction);

	bool operator==(const FGothamSettingsData& Other) const;
	bool operator!=(const FGothamSettingsData& Other) const { return !(*this == Other); }

	/** Persistence in a config file section. Unknown or out-of-range values fall back to defaults. */
	void LoadFromConfig(const FConfigFile& File, const TCHAR* Section);
	void SaveToConfig(FConfigFile& File, const TCHAR* Section) const;
};

/** Semantic colours. Widgets ask for a token, never a literal colour, so palettes can change under them. */
enum class EGothamColorToken : uint8
{
	Good,       // healthy, complete
	Danger,     // low health, warnings that need action
	Info,       // neutral highlights, detective UI
	Warning,    // combo meter, cautions
	Unscanned,  // clue not yet investigated
	Scanned,    // clue investigated
	// Neutral tokens: the same in every colour-vision preset, tightened in high contrast.
	Panel,        // panel fill (use with PanelAlpha)
	PanelEdge,    // thin panel borders and rules
	TextPrimary,  // main text
	TextMuted,    // secondary text and labels
	Accent,       // the one warm highlight: selection, focus, the combo multiplier
};

namespace GothamPalette
{
	MVVMSAMPLE_API FLinearColor Resolve(EGothamColorToken Token, EGothamColorMode Mode, bool bHighContrast);

	/** Opacity of panel and dim backgrounds. Higher in high-contrast mode so text always has a solid backdrop. */
	MVVMSAMPLE_API float PanelAlpha(bool bHighContrast);
}
