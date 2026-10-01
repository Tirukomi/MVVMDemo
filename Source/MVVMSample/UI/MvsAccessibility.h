// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"

class SWidget;
class UWidget;

/**
 * What screen readers announce. Slate reads a widget's accessible text when it gets focus (or is hovered); by default
 * that is a summary of the text inside it, which for a settings row is a run of fragments and for custom-painted Slate
 * (the wheel, the markers) is nothing. These give such widgets a real sentence.
 *
 * Set on the Slate widget, which is what the reader sees: UMG's own accessibility properties only reach Slate in
 * editor builds.
 */
namespace MvsAccessibility
{
	/** Readers announce Text for Widget, and do not walk into its children separately. */
	MVVMSAMPLE_API void SetText(const TSharedPtr<SWidget>& Widget, TAttribute<FText> Text);

	/** The focusable button inside a Common UI button: what a reader lands on when the item has focus. */
	MVVMSAMPLE_API TSharedPtr<SWidget> FindButton(const UWidget& Widget);

	/** The text a reader would announce for Widget (for tests). */
	MVVMSAMPLE_API FText GetText(const TSharedPtr<SWidget>& Widget);
}
