// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Components/TextBlock.h"
#include "UI/Style/MvsStyle.h"
#include "MvsText.generated.h"

/** EMvsTextStyle for the widget designer (that enum is not reflected), with "None" for "leave the font as set". */
UENUM()
enum class EMvsDesignerTextStyle : uint8
{
	None,
	Display,
	Title,
	Header,
	Label,
	Numeric,
	Body,
	BodyStrong,
	Key,
};

/** EMvsColorToken for the widget designer, with "None" for "leave the colour as set". */
UENUM()
enum class EMvsDesignerColor : uint8
{
	None,
	Good,
	Danger,
	Info,
	Warning,
	Unscanned,
	Scanned,
	Panel,
	PanelEdge,
	TextPrimary,
	TextMuted,
	Accent,
};

/**
 * A text block that can show its text in capitals. Slate's ETextTransformPolicy::ToUpper assumes upper-casing never
 * changes a string's length, which is false in German ("ß" becomes "SS"): it ensures and leaves the text in mixed
 * case. This upper-cases the FText itself instead (FText::ToUpper is culture-aware and follows live language
 * switches), and keeps the original so the style can switch back.
 *
 * It also styles itself: given a type style (and optionally a palette token) it keeps its font at the player's Text
 * size and its colour in the current palette, re-applying both when settings change. Nothing has to recolour it.
 *
 * Code-built widgets construct this instead of UTextBlock; members can stay UTextBlock* because SetText is virtual.
 * In the widget designer, Style and Color under "Mvs" do the same (second review 11): a designer-built text then
 * looks and restyles exactly like a code-built one, capitals included where the style uses them.
 */
UCLASS()
class MVVMSAMPLE_API UMvsText : public UTextBlock
{
	GENERATED_BODY()

public:
	virtual void SetText(FText InText) override;

	void SetUpperCase(bool bInUpperCase);
	bool IsUpperCase() const { return bUpperCase; }

	/** The text as set, before upper-casing. */
	const FText& GetSourceText() const { return SourceText; }

	/** Keeps the font at Style, scaled by the Text size setting. */
	void SetTextStyle(EMvsTextStyle InStyle);
	/** Keeps the colour at a palette token. Code that sets the colour itself does not call this. */
	void SetColorToken(EMvsColorToken InToken, float InAlpha = 1.f);

	/** Designer setting: the type style the text keeps (and its capitals). None leaves the font as set. */
	UPROPERTY(EditAnywhere, Category = "Mvs")
	EMvsDesignerTextStyle Style = EMvsDesignerTextStyle::None;

	/** Designer setting: the palette colour the text keeps. None leaves the colour as set. */
	UPROPERTY(EditAnywhere, Category = "Mvs")
	EMvsDesignerColor Color = EMvsDesignerColor::None;

	UPROPERTY(EditAnywhere, Category = "Mvs", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "Color != EMvsDesignerColor::None"))
	float ColorOpacity = 1.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	void ApplyText();
	/** Font and colour from the current theme, for whatever was set with SetTextStyle / SetColorToken. */
	/** Font and colour from the current theme; does nothing if that theme was already applied (unless forced). */
	void ApplyTheme(bool bForce = false);

	TOptional<EMvsTextStyle> TextStyle;
	TOptional<EMvsColorToken> ColorToken;
	float ColorAlpha = 1.f;
	TOptional<FMvsTheme> AppliedTheme;
	FMvsSettingsListener SettingsListener;

	FText SourceText;
	bool bUpperCase = false;
	bool bHasSource = false;
};

namespace MvsText
{
	/** Capitals on or off for any text block: UMvsText upper-cases its text, a plain UTextBlock falls back to Slate's transform. */
	MVVMSAMPLE_API void SetUpperCase(UTextBlock* Text, bool bUpperCase);
}
