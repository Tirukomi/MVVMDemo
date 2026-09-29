// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamPanel.h"

#include "Components/PanelSlot.h"
#include "UI/Slate/SGothamPanel.h"
#include "Widgets/SNullWidget.h"

TSharedRef<SWidget> UGothamPanel::RebuildWidget()
{
	MyPanel = SNew(SGothamPanel)
		.Padding(Padding)
		.Corner(Corner)
		.ChamferMask(ChamferMask)
		.FillColor(FillColor)
		.EdgeColor(EdgeColor)
		.EdgeThickness(EdgeThickness)
		.AccentColor(AccentColor)
		.AccentWidth(AccentWidth);

	if (GetChildrenCount() > 0 && GetContentSlot()->Content)
	{
		MyPanel->SetContent(GetContentSlot()->Content->TakeWidget());
	}
	return MyPanel.ToSharedRef();
}

void UGothamPanel::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MyPanel.IsValid() && InSlot->Content)
	{
		MyPanel->SetContent(InSlot->Content->TakeWidget());
	}
}

void UGothamPanel::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MyPanel.IsValid())
	{
		MyPanel->SetContent(SNullWidget::NullWidget);
	}
}

void UGothamPanel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MyPanel.IsValid())
	{
		MyPanel->SetPadding(Padding);
		MyPanel->SetShape(Corner, ChamferMask);
		MyPanel->SetColors(FillColor, EdgeColor, EdgeThickness);
		MyPanel->SetAccent(AccentColor, AccentWidth);
	}
}

void UGothamPanel::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyPanel.Reset();
}

void UGothamPanel::SetShape(float InCorner, uint8 InMask)
{
	Corner = InCorner;
	ChamferMask = InMask;
	if (MyPanel.IsValid()) { MyPanel->SetShape(Corner, ChamferMask); }
}

void UGothamPanel::SetColors(const FLinearColor& InFill, const FLinearColor& InEdge, float InEdgeThickness)
{
	FillColor = InFill;
	EdgeColor = InEdge;
	EdgeThickness = InEdgeThickness;
	if (MyPanel.IsValid()) { MyPanel->SetColors(FillColor, EdgeColor, EdgeThickness); }
}

void UGothamPanel::SetAccent(const FLinearColor& InColor, float InWidth)
{
	AccentColor = InColor;
	AccentWidth = InWidth;
	if (MyPanel.IsValid()) { MyPanel->SetAccent(AccentColor, AccentWidth); }
}

void UGothamPanel::SetPanelPadding(const FMargin& InPadding)
{
	Padding = InPadding;
	if (MyPanel.IsValid()) { MyPanel->SetPadding(Padding); }
}
