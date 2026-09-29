// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Fonts/SlateFontInfo.h"

class UTextBlock;

/** The type scale. Every text block picks one of these instead of setting a font size by hand. */
enum class EGothamTextStyle : uint8
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
 * Fonts and text styling. Fonts are composite fonts built from the OFL files in Content/UI/Fonts (Barlow Condensed for
 * display and labels, Barlow for body) with Noto Sans JP as a sub-font for Japanese ranges, so every style renders CJK.
 * Built in code from the TTF files: no font assets to import, and missing files fall back to the engine font.
 */
namespace GothamStyle
{
	MVVMSAMPLE_API FSlateFontInfo Font(EGothamTextStyle Style);

	/** Whether the style is shown upper-case (labels and titles). */
	MVVMSAMPLE_API bool IsUpperCase(EGothamTextStyle Style);

	/** Applies font, case transform and colour. */
	MVVMSAMPLE_API void ApplyText(UTextBlock* Text, EGothamTextStyle Style, const FLinearColor& Color);

	/** A palette token under the player's current colour and contrast settings (defaults without a game instance). */
	MVVMSAMPLE_API FLinearColor Token(const UObject* Context, EGothamColorToken InToken, float Alpha = 1.f);

	/** Panel fill alpha under the current contrast setting. */
	MVVMSAMPLE_API float PanelAlpha(const UObject* Context);

	/** Convenience: construct-and-style is the common case in code-built widgets. */
	MVVMSAMPLE_API bool AreCustomFontsAvailable();
}
