// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamMenuList.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Layout/WidgetPath.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Slate/SGothamHighlight.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Style/GothamStyle.h"

TSharedRef<SWidget> UGothamHighlightBar::RebuildWidget()
{
	MyHighlight = SNew(SGothamHighlight);
	return MyHighlight.ToSharedRef();
}

void UGothamHighlightBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyHighlight.Reset();
}

namespace
{
	/** Builds the list's tree on first use: items are usually added before the list is ever shown. */
	void EnsureTree(UWidgetTree* Tree, TObjectPtr<UGothamHighlightBar>& Highlight, TObjectPtr<UVerticalBox>& Box)
	{
		if (Tree->RootWidget)
		{
			return;
		}
		UOverlay* Root = Tree->ConstructWidget<UOverlay>();
		Tree->RootWidget = Root;
		Highlight = Tree->ConstructWidget<UGothamHighlightBar>();
		UOverlaySlot* BarSlot = Root->AddChildToOverlay(Highlight);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
		Box = Tree->ConstructWidget<UVerticalBox>();
		Root->AddChildToOverlay(Box)->SetHorizontalAlignment(HAlign_Fill);
	}
}

UVerticalBoxSlot* UGothamMenuList::AddItem(UWidget* Item)
{
	EnsureTree(WidgetTree, Highlight, Box);
	Items.Add(Item);
	UVerticalBoxSlot* ItemSlot = Box->AddChildToVerticalBox(Item);
	ItemSlot->SetHorizontalAlignment(HAlign_Fill);
	ItemSlot->SetPadding(FMargin(0.f, 2.f));
	return ItemSlot;
}

TSharedRef<SWidget> UGothamMenuList::RebuildWidget()
{
	EnsureTree(WidgetTree, Highlight, Box);
	return Super::RebuildWidget();
}

void UGothamMenuList::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyLook(); });
	ApplyLook();
}

void UGothamMenuList::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamMenuList::ApplyLook()
{
	const TSharedPtr<SGothamHighlight> Bar = Highlight ? Highlight->GetSlate() : nullptr;
	if (!Bar.IsValid())
	{
		return;
	}
	using namespace GothamStyle;
	const bool bHighContrast = PanelAlpha(this) > 0.9f;
	FGothamPanelLook Look;
	Look.Corner = 10.f;
	Look.ChamferMask = EGothamChamfer::Opposite;
	Look.Fill = Token(this, EGothamColorToken::Accent, bHighContrast ? 0.3f : 0.14f);
	Look.Edge = Token(this, EGothamColorToken::Accent, bHighContrast ? 1.f : 0.45f);
	Look.EdgeThickness = 1.f;
	Look.Accent = Token(this, EGothamColorToken::Accent);
	Look.AccentWidth = 4.f;
	Look.Glow = Token(this, EGothamColorToken::Accent, 0.14f);
	Look.GlowSize = 6.f;
	Bar->SetLook(Look);
	Bar->SetReducedMotion(GothamMotion::IsReduced(this));
}

void UGothamMenuList::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
	const TSharedPtr<SGothamHighlight> Bar = Highlight ? Highlight->GetSlate() : nullptr;
	if (!Bar.IsValid())
	{
		return;
	}
	for (UWidget* Item : Items)
	{
		const TSharedPtr<SWidget> ItemSlate = Item ? Item->GetCachedWidget() : nullptr;
		if (ItemSlate.IsValid() && NewWidgetPath.ContainsWidget(ItemSlate.Get()))
		{
			Bar->SetTarget(ItemSlate, false);
			Bar->SetActive(true);
			if (CurrentItem.Get() != Item)
			{
				CurrentItem = Item;
				OnCurrentItemChanged.Broadcast(Item);
			}
			return;
		}
	}
	// Focus moved somewhere outside the items (e.g. the action buttons): keep the bar where it was, dimmed.
	Bar->SetActive(false);
}
