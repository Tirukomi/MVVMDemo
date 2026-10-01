// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsHudPrimitives.h"

#include "UI/Slate/SDamageVignette.h"
#include "UI/Slate/SGadgetIcon.h"

namespace
{
	EMvsGadgetIcon ToIcon(int32 Index)
	{
		return static_cast<EMvsGadgetIcon>(FMath::Clamp(Index, 0, static_cast<int32>(EMvsGadgetIcon::Count) - 1));
	}
}

TSharedRef<SWidget> UGadgetIcon::RebuildWidget()
{
	SlateIcon = SNew(SGadgetIcon).Icon(ToIcon(IconIndex)).Size(IconSize).Color(Color).RingColor(RingColor);
	SlateIcon->SetCooldown(Cooldown);
	return SlateIcon.ToSharedRef();
}

void UGadgetIcon::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateIcon.IsValid())
	{
		SlateIcon->SetIcon(ToIcon(IconIndex));
		SlateIcon->SetSize(IconSize);
		SlateIcon->SetColors(Color, RingColor);
		SlateIcon->SetCooldown(Cooldown);
	}
}

void UGadgetIcon::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateIcon.Reset();
}

void UGadgetIcon::SetIconIndex(int32 InIndex)
{
	IconIndex = InIndex;
	if (SlateIcon.IsValid()) { SlateIcon->SetIcon(ToIcon(IconIndex)); }
}

void UGadgetIcon::SetColors(const FLinearColor& InColor, const FLinearColor& InRing)
{
	Color = InColor;
	RingColor = InRing;
	if (SlateIcon.IsValid()) { SlateIcon->SetColors(Color, RingColor); }
}

void UGadgetIcon::SetCooldown(float InPercent)
{
	Cooldown = InPercent;
	if (SlateIcon.IsValid()) { SlateIcon->SetCooldown(Cooldown); }
}

TSharedRef<SWidget> UDamageVignette::RebuildWidget()
{
	SlateVignette = SNew(SDamageVignette).Color(Color);
	SlateVignette->SetIntensity(Intensity);
	return SlateVignette.ToSharedRef();
}

void UDamageVignette::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateVignette.Reset();
}

void UDamageVignette::SetIntensity(float InIntensity)
{
	Intensity = InIntensity;
	if (SlateVignette.IsValid()) { SlateVignette->SetIntensity(Intensity); }
}

void UDamageVignette::SetColor(const FLinearColor& InColor)
{
	Color = InColor;
	if (SlateVignette.IsValid()) { SlateVignette->SetColor(Color); }
}
