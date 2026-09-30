// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "GothamText.generated.h"

/**
 * A text block that can show its text in capitals. Slate's ETextTransformPolicy::ToUpper assumes upper-casing never
 * changes a string's length, which is false in German ("ß" becomes "SS"): it ensures and leaves the text in mixed
 * case. This upper-cases the FText itself instead (FText::ToUpper is culture-aware and follows live language
 * switches), and keeps the original so the style can switch back.
 *
 * Code-built widgets construct this instead of UTextBlock; members can stay UTextBlock* because SetText is virtual.
 */
UCLASS()
class MVVMSAMPLE_API UGothamText : public UTextBlock
{
	GENERATED_BODY()

public:
	virtual void SetText(FText InText) override;

	void SetUpperCase(bool bInUpperCase);
	bool IsUpperCase() const { return bUpperCase; }

	/** The text as set, before upper-casing. */
	const FText& GetSourceText() const { return SourceText; }

private:
	void ApplyText();

	FText SourceText;
	bool bUpperCase = false;
	bool bHasSource = false;
};

namespace GothamText
{
	/** Capitals on or off for any text block: UGothamText upper-cases its text, a plain UTextBlock falls back to Slate's transform. */
	MVVMSAMPLE_API void SetUpperCase(UTextBlock* Text, bool bUpperCase);
}
