// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsScrim.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SMvsScrim : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMvsScrim) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		SetCanTick(false);
	}

	void Set(const FLinearColor& InColor, float InLeft, float InRight)
	{
		Color = InColor;
		Left = InLeft;
		Right = InRight;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out,
		int32 LayerId, const FWidgetStyle& Style, bool) const override
	{
		const FVector2f Size = FVector2f(Geometry.GetLocalSize());
		const float Opacity = Style.GetColorAndOpacityTint().A;
		auto Vertex = [&](const FVector2f& Local, float Alpha)
		{
			FSlateVertex V;
			V.Position = FVector2f(Geometry.LocalToAbsolute(FVector2D(Local)));
			FLinearColor C = Color;
			C.A = Alpha * Opacity;
			V.Color = C.ToFColor(true);
			return V;
		};
		// Three columns of vertices: the dark plateau covers the left 40%, then it eases toward the right alpha.
		const float Knee = Size.X * 0.4f;
		TArray<FSlateVertex> Verts = {
			Vertex({ 0.f, 0.f }, Left), Vertex({ Knee, 0.f }, Left), Vertex({ Size.X, 0.f }, Right),
			Vertex({ 0.f, Size.Y }, Left), Vertex({ Knee, Size.Y }, Left), Vertex({ Size.X, Size.Y }, Right),
		};
		TArray<SlateIndex> Indices = { 0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4 };
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
		FSlateDrawElement::MakeCustomVerts(Out, LayerId, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
		return LayerId + 1;
	}

private:
	FLinearColor Color;
	float Left = 0.9f;
	float Right = 0.5f;
};

TSharedRef<SWidget> UMvsScrim::RebuildWidget()
{
	MyScrim = SNew(SMvsScrim);
	MyScrim->Set(Color, LeftAlpha, RightAlpha);
	return MyScrim.ToSharedRef();
}

void UMvsScrim::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyScrim.IsValid()) { MyScrim->Set(Color, LeftAlpha, RightAlpha); }
}

void UMvsScrim::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyScrim.Reset();
}
