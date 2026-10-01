// Copyright IG. All Rights Reserved.

#include "UI/Widgets/GadgetWheel.h"

#include "UI/Slate/SGadgetWheel.h"

TSharedRef<SWidget> UGadgetWheel::RebuildWidget()
{
	SlateWheel = SNew(SGadgetWheel)
		.Style(WheelStyle)
		.OnItemSelected(FOnWheelIndex::CreateWeakLambda(this, [this](int32 Index) { OnItemSelected.Broadcast(Index); }))
		.OnItemHovered(FOnWheelIndex::CreateWeakLambda(this, [this](int32 Index) { OnItemHovered.Broadcast(Index); }));
	SlateWheel->SetReduceMotion(bReduceMotion);
	SlateWheel->SetItems(Items);
	return SlateWheel.ToSharedRef();
}

void UGadgetWheel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateWheel.IsValid())
	{
		SlateWheel->SetReduceMotion(bReduceMotion);
		SlateWheel->SetStyle(WheelStyle);
		SlateWheel->SetItems(Items);
	}
}

void UGadgetWheel::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateWheel.Reset();
}

void UGadgetWheel::SetItems(const TArray<FMvsWheelItem>& InItems)
{
	Items = InItems;
	if (SlateWheel.IsValid())
	{
		SlateWheel->SetItems(Items);
	}
}

void UGadgetWheel::SetStickInput(FVector2D Stick)
{
	if (SlateWheel.IsValid())
	{
		SlateWheel->SetStickInput(Stick);
	}
}

int32 UGadgetWheel::GetHoveredIndex() const
{
	return SlateWheel.IsValid() ? SlateWheel->GetHoveredIndex() : INDEX_NONE;
}

bool UGadgetWheel::CommitHovered()
{
	return SlateWheel.IsValid() && SlateWheel->CommitHovered();
}
