// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Fonts/SlateFontInfo.h"

class UTextBlock;

/** The type scale. Every text block picks one of these instead of setting a font size by hand. */
enum class EMvsTextStyle : uint8
{
	Display,     // big combo numerals
	Title,       // screen titles
	Header,      // panel headers, selected gadget name
	Label,       // small caps labels ("OBJECTIVE", "HITS"), letter-spaced
	Numeric,     // counters and readouts
	Body,        // sentences
	BodyStrong,  // emphasised sentences, button labels
	Key,         // key-cap and button glyph text
};

/**
 * The look the player's settings ask for, resolved once: palette (colour mode, contrast), panel opacity, motion and
 * text size. The one place UI code reads style from; widgets that restyle themselves take it from MvsStyle::Theme.
 */
struct MVVMSAMPLE_API FMvsTheme
{
	EMvsColorMode ColorMode = EMvsColorMode::Default;
	bool bHighContrast = false;
	bool bReducedMotion = false;
	/** Multiplies every font size (the Text size setting), on top of UI scale. */
	float TextScale = 1.f;

	static FMvsTheme FromSettings(const FMvsSettingsData& Data);

	bool operator==(const FMvsTheme& Other) const
	{
		return ColorMode == Other.ColorMode && bHighContrast == Other.bHighContrast && bReducedMotion == Other.bReducedMotion && TextScale == Other.TextScale;
	}

	FLinearColor Color(EMvsColorToken Token, float Alpha = 1.f) const;
	/** Panel fill alpha under the contrast setting. */
	float PanelAlpha() const;
	/** A type-scale font at this theme's text size. */
	FSlateFontInfo Font(EMvsTextStyle Style) const;
};

/**
 * Fonts and text styling. Fonts are composite fonts built from the OFL files in Content/UI/Fonts (Barlow Condensed for
 * display and labels, Barlow for body) with Noto Sans JP as a sub-font for Japanese ranges, so every style renders CJK.
 * Built in code from the TTF files: no font assets to import, and missing files fall back to the engine font.
 */
namespace MvsStyle
{
	/** The theme for the player's current settings (defaults without a game instance). */
	MVVMSAMPLE_API FMvsTheme Theme(const UObject* Context);

	/** A type-scale font at the default text size (custom Slate layers; UMG text follows the theme's text size). */
	MVVMSAMPLE_API FSlateFontInfo Font(EMvsTextStyle Style, float Scale = 1.f);

	/** Whether the style is shown upper-case (labels and titles). */
	MVVMSAMPLE_API bool IsUpperCase(EMvsTextStyle Style);

	/** The font only: a UMvsText keeps the style (and follows the Text size setting), other text blocks get it once. */
	MVVMSAMPLE_API void SetTextStyle(UTextBlock* Text, EMvsTextStyle Style);

	/**
	 * Applies font, case transform and colour. A UMvsText also keeps the style, so its font follows the Text size
	 * setting from then on.
	 */
	MVVMSAMPLE_API void ApplyText(UTextBlock* Text, EMvsTextStyle Style, const FLinearColor& Color);

	/** A palette token under the player's current colour and contrast settings (defaults without a game instance). */
	MVVMSAMPLE_API FLinearColor Token(const UObject* Context, EMvsColorToken InToken, float Alpha = 1.f);
	/** The label colour every menu item shares: primary while focused or hovered ("hot"), muted otherwise. */
	MVVMSAMPLE_API FLinearColor ItemText(const UObject* Context, bool bHot);

	/** Panel fill alpha under the current contrast setting. */
	MVVMSAMPLE_API float PanelAlpha(const UObject* Context);

}
