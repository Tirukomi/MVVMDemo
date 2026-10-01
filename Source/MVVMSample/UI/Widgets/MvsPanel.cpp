// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsPanel.h"

#include "Components/PanelSlot.h"
#include "UI/Slate/SMvsPanel.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UMvsPanel::RebuildWidget()
{
	MyPanel = SNew(SMvsPanel)
		.Padding(Padding)
		.Corner(Corner)
		.ChamferMask(ChamferMask)
		.FillColor(FillColor)
		.EdgeColor(EdgeColor)
		.EdgeThickness(EdgeThickness)
		.AccentColor(AccentColor)
		.AccentWidth(AccentWidth);
	MyPanel->SetGlow(GlowColor, GlowSize);

	if (GetChildrenCount() > 0 && GetContentSlot()->Content)
	{
		MyPanel->SetContent(GetContentSlot()->Content->TakeWidget());
	}
	return MyPanel.ToSharedRef();
}

void UMvsPanel::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MyPanel.IsValid() && InSlot->Content)
	{
		MyPanel->SetContent(InSlot->Content->TakeWidget());
	}
}

void UMvsPanel::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MyPanel.IsValid())
	{
		MyPanel->SetContent(SNullWidget::NullWidget);
	}
}

void UMvsPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyPanel.IsValid())
	{
		MyPanel->SetPadding(Padding);
		MyPanel->SetShape(Corner, ChamferMask);
		MyPanel->SetColors(FillColor, EdgeColor, EdgeThickness);
		MyPanel->SetAccent(AccentColor, AccentWidth);
		MyPanel->SetGlow(GlowColor, GlowSize);
	}
}

void UMvsPanel::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyPanel.Reset();
}

void UMvsPanel::SetShape(float InCorner, uint8 InMask)
{
	Corner = InCorner;
	ChamferMask = InMask;
	if (MyPanel.IsValid()) { MyPanel->SetShape(Corner, ChamferMask); }
}

void UMvsPanel::SetColors(const FLinearColor& InFill, const FLinearColor& InEdge, float InEdgeThickness)
{
	FillColor = InFill;
	EdgeColor = InEdge;
	EdgeThickness = InEdgeThickness;
	if (MyPanel.IsValid()) { MyPanel->SetColors(FillColor, EdgeColor, EdgeThickness); }
}

void UMvsPanel::SetAccent(const FLinearColor& InColor, float InWidth)
{
	AccentColor = InColor;
	AccentWidth = InWidth;
	if (MyPanel.IsValid()) { MyPanel->SetAccent(AccentColor, AccentWidth); }
}

void UMvsPanel::SetGlow(const FLinearColor& InColor, float InSize)
{
	GlowColor = InColor;
	GlowSize = InSize;
	if (MyPanel.IsValid()) { MyPanel->SetGlow(GlowColor, GlowSize); }
}

void UMvsPanel::SetPanelPadding(const FMargin& InPadding)
{
	Padding = InPadding;
	if (MyPanel.IsValid()) { MyPanel->SetPadding(Padding); }
}
