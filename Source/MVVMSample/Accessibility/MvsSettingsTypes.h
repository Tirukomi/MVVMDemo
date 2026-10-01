// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"

/** Colour-vision presets. Each maps the semantic colour tokens to a palette that stays distinguishable. */
enum class EMvsColorMode : uint8
{
	Default,
	Protanopia,
	Deuteranopia,
	Tritanopia,
	Count
};

/** Text size for every menu and HUD label, separate from UI scale (which scales layout too). */
enum class EMvsTextSize : uint8
{
	Standard,
	Large,
	Larger,
	Count
};

enum class EMvsSubtitleSize : uint8
{
	Small,
	Medium,
	Large,
	Count
};

/** How clue analysis is triggered: hold the scan input for a moment, or tap once. */
enum class EMvsScanMode : uint8
{
	Hold,
	Tap,
	Count
};

enum class EMvsWheelMode : uint8
{
	Hold,
	Toggle,
	Count
};

/** Every option the settings screen edits. */
enum class EMvsSetting : uint8
{
	Language,
	ColorVision,
	UIScale,
	TextSize,
	HighContrast,
	ReducedMotion,
	WheelMode,
	ScanMode,
	SubtitleSize,
	SubtitleBackground,
	Count
};

/** A language the player can pick. Names are in their own language on purpose, so they are never translated. */
struct FMvsLanguageOption
{
	const TCHAR* Culture;
	const TCHAR* NativeName;
};

/** The plain data behind the settings screen. No UObjects, so rules and persistence are unit-testable. */
struct MVVMSAMPLE_API FMvsSettingsData
{
	FString Language = TEXT("en");
	EMvsColorMode ColorMode = EMvsColorMode::Default;
	int32 UIScaleIndex = 1;
	EMvsTextSize TextSize = EMvsTextSize::Standard;
	bool bHighContrast = false;
	bool bReducedMotion = false;
	EMvsWheelMode WheelMode = EMvsWheelMode::Hold;
	EMvsScanMode ScanMode = EMvsScanMode::Hold;
	EMvsSubtitleSize SubtitleSize = EMvsSubtitleSize::Medium;
	bool bSubtitleBackground = true;

	static const TArray<float>& GetUIScaleSteps();
	static const TArray<FMvsLanguageOption>& GetLanguages();

	float GetUIScale() const;
	/** What text size multiplies font sizes by (on top of UI scale). */
	float GetTextScale() const;
	int32 GetSubtitleFontSize() const;

	/** Where the current value sits among the option's choices, for the selector's position pips. */
	void GetOptionPosition(EMvsSetting Setting, int32& OutIndex, int32& OutCount) const;

	/** Steps a setting by Direction (+1 / -1), wrapping. Returns true if the value changed. */
	bool Cycle(EMvsSetting Setting, int32 Direction);

	bool operator==(const FMvsSettingsData& Other) const;
	bool operator!=(const FMvsSettingsData& Other) const { return !(*this == Other); }

	/** Persistence in a config file section. Unknown or out-of-range values fall back to defaults. */
	void LoadFromConfig(const FConfigFile& File, const TCHAR* Section);
	void SaveToConfig(FConfigFile& File, const TCHAR* Section) const;
};

/** Semantic colours. Widgets ask for a token, never a literal colour, so palettes can change under them. */
enum class EMvsColorToken : uint8
{
	Good,       // healthy, complete
	Danger,     // low health, warnings that need action
	Info,       // neutral highlights, forensic UI
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

namespace MvsPalette
{
	MVVMSAMPLE_API FLinearColor Resolve(EMvsColorToken Token, EMvsColorMode Mode, bool bHighContrast);

	/** Opacity of panel and dim backgrounds. Higher in high-contrast mode so text always has a solid backdrop. */
	MVVMSAMPLE_API float PanelAlpha(bool bHighContrast);
}
