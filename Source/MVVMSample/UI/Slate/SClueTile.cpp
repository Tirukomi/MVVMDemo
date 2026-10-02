// Copyright IG. All Rights Reserved.

#include "UI/Slate/SClueTile.h"

#include "Layout/ArrangedChildren.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Text/STextBlock.h"

SClueTile::SClueTile()
	: Children(this)
{
}

void SClueTile::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	Children.Add(SAssignNew(Mark, STextBlock));
	Children.Add(SAssignNew(Number, STextBlock));
	Children.Add(SAssignNew(Title, STextBlock).Clipping(EWidgetClipping::ClipToBounds));
}

void SClueTile::SetLook(const FMvsPanelLook& InLook)
{
	Look = InLook;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SClueTile::SetThumbnail(const FSlateBrush& InBrush, const FLinearColor& InTint)
{
	ThumbnailBrush = InBrush;
	ThumbnailTint = InTint;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FSlateRect SClueTile::PictureRect(const FVector2D& Size) const
{
	const float TitleHeight = Title->GetDesiredSize().Y;
	const float Bottom = FMath::Max(Padding, static_cast<float>(Size.Y) - Padding - TitleHeight - TitleGap);
	return FSlateRect(Padding, Padding, static_cast<float>(Size.X) - Padding, Bottom);
}

void SClueTile::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FSlateRect Picture = PictureRect(Size);
	const FVector2D PictureSize = Picture.GetSize();

	// The "?": centred in the picture, at its desired size.
	if (ArrangedChildren.Accepts(Mark->GetVisibility()))
	{
		const FVector2D Desired = Mark->GetDesiredSize();
		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Mark.ToSharedRef(),
			FVector2f(Picture.GetTopLeft() + (PictureSize - Desired) * 0.5), FVector2f(Desired)));
	}
	// The case number: fills the picture inside its padding (the text draws at the top left).
	if (ArrangedChildren.Accepts(Number->GetVisibility()))
	{
		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Number.ToSharedRef(),
			FVector2f(Picture.Left + NumberPaddingX, Picture.Top + NumberPaddingY),
			FVector2f(FMath::Max(0.0, PictureSize.X - 2.0 * NumberPaddingX), FMath::Max(0.0, PictureSize.Y - 2.0 * NumberPaddingY))));
	}
	// The title: the width of the content, below the picture.
	if (ArrangedChildren.Accepts(Title->GetVisibility()))
	{
		ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(Title.ToSharedRef(),
			FVector2f(Padding + TitleIndent, Picture.Bottom + TitleGap),
			FVector2f(FMath::Max(0.0, Size.X - 2.0 * Padding - TitleIndent), Title->GetDesiredSize().Y)));
	}
}

FVector2D SClueTile::ComputeDesiredSize(float) const
{
	// The tile view gives every tile the same slot; this is the content's minimum.
	return FVector2D(2.0 * Padding, 2.0 * Padding + Title->GetDesiredSize().Y + TitleGap);
}

int32 SClueTile::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	MvsPaintPanel(OutDrawElements, LayerId, AllottedGeometry, FVector2f::ZeroVector, FVector2f(Size), Look, Opacity);

	const FSlateRect Picture = PictureRect(Size);
	FLinearColor Tint = ThumbnailTint;
	Tint.A *= Opacity;
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 2,
		AllottedGeometry.ToPaintGeometry(FVector2f(Picture.GetSize()), FSlateLayoutTransform(FVector2f(Picture.GetTopLeft()))),
		&ThumbnailBrush, ESlateDrawEffect::None, Tint);

	FArrangedChildren Arranged(EVisibility::Visible);
	ArrangeChildren(AllottedGeometry, Arranged);
	return PaintArrangedChildren(Args, Arranged, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 3, InWidgetStyle, ShouldBeEnabled(bParentEnabled));
}
