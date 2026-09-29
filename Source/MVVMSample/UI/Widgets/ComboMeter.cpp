// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/ComboMeter.h"

#include "UI/Slate/SComboMeter.h"

TSharedRef<SWidget> UComboMeter::RebuildWidget()
{
	SlateMeter = SNew(SComboMeter)
		.SegmentCount(SegmentCount)
		.DesiredSize(MeterSize)
		.FilledColor(FilledColor)
		.EmptyColor(EmptyColor)
		.GhostColor(GhostColor)
		.Skew(Skew)
		.Gap(Gap);
	SlateMeter->SetPercent(Percent);
	return SlateMeter.ToSharedRef();
}

void UComboMeter::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateMeter.IsValid())
	{
		SlateMeter->SetReduceMotion(bReduceMotion);
		SlateMeter->SetSegmentCount(SegmentCount);
		SlateMeter->SetDesiredSize(MeterSize);
		SlateMeter->SetColors(FilledColor, EmptyColor);
		SlateMeter->SetGhostColor(GhostColor);
		SlateMeter->SetShape(Skew, Gap);
		SlateMeter->SetPercent(Percent);
	}
}

void UComboMeter::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateMeter.Reset();
}

void UComboMeter::SetPercent(float InPercent)
{
	Percent = InPercent;
	if (SlateMeter.IsValid())
	{
		SlateMeter->SetPercent(Percent);
	}
}
