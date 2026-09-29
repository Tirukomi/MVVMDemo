// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Layout/GothamPrimaryLayout.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "UI/GothamWidgetTick.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

TSharedRef<SWidget> UGothamPrimaryLayout::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Layers.Reset();
		for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Count); ++i)
		{
			UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>();
			UOverlaySlot* StackSlot = Root->AddChildToOverlay(Stack);
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Fill);
			Layers.Add(Stack);
		}
	}
	return Super::RebuildWidget();
}

void UGothamPrimaryLayout::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
}

UCommonActivatableWidgetStack* UGothamPrimaryLayout::GetLayer(EGothamUILayer Layer) const
{
	return Layers.IsValidIndex(static_cast<int32>(Layer)) ? Layers[static_cast<int32>(Layer)].Get() : nullptr;
}
