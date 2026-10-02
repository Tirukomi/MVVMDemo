// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/Slate/SMvsPanel.h"
#include "Widgets/SPanel.h"

class STextBlock;

/**
 * One case-file tile in a single panel (S7): the chamfered frame and the thumbnail are painted, and three text blocks
 * (the "?" of an undiscovered clue, the case number, the title) are laid out by hand. The tile view clears and re-adds
 * every visible tile on each frame of a scroll, so a tile's widget count is paid per tile per frame; this replaces a
 * size box, panel, box, vertical box, overlay and image with one widget, keeping the text as text blocks so it renders
 * exactly as before.
 *
 * Layout (the same arithmetic as the containers it replaces): inside Padding, the picture fills the space above the
 * title; the title sits TitleGap below it with TitleIndent on the left; the case number is inset NumberPadding in the
 * picture's top-left, the "?" is centred in the picture.
 */
class MVVMSAMPLE_API SClueTile : public SPanel
{
public:
	SLATE_BEGIN_ARGS(SClueTile) {}
	SLATE_END_ARGS()

	SClueTile();
	void Construct(const FArguments& InArgs);

	void SetLook(const FMvsPanelLook& InLook);
	void SetThumbnail(const FSlateBrush& InBrush, const FLinearColor& InTint);
	STextBlock& GetMark() const { return *Mark; }
	STextBlock& GetNumber() const { return *Number; }
	STextBlock& GetTitle() const { return *Title; }

	static constexpr float Padding = 7.f;
	static constexpr float TitleGap = 6.f;
	static constexpr float TitleIndent = 1.f;
	static constexpr float NumberPaddingX = 5.f;
	static constexpr float NumberPaddingY = 3.f;

	virtual void OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual FChildren* GetChildren() override { return &Children; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	/** The picture's rectangle in local space, given the title's height. */
	FSlateRect PictureRect(const FVector2D& Size) const;

	TSlotlessChildren<SWidget> Children;
	TSharedPtr<STextBlock> Mark;
	TSharedPtr<STextBlock> Number;
	TSharedPtr<STextBlock> Title;
	FMvsPanelLook Look;
	FSlateBrush ThumbnailBrush;
	FLinearColor ThumbnailTint = FLinearColor::White;
};
