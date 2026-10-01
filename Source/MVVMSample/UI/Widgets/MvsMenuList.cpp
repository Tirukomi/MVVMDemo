// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsMenuList.h"

#include "Accessibility/MvsSettingsListener.h"
#include "Accessibility/MvsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Layout/WidgetPath.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Slate/SMvsHighlight.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Style/MvsStyle.h"

TSharedRef<SWidget> UMvsHighlightBar::RebuildWidget()
{
	MyHighlight = SNew(SMvsHighlight);
	return MyHighlight.ToSharedRef();
}

void UMvsHighlightBar::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyHighlight.Reset();
}

namespace
{
	/** Builds the list's tree on first use: items are usually added before the list is ever shown. */
	void EnsureTree(UWidgetTree* Tree, TObjectPtr<UMvsHighlightBar>& Highlight, TObjectPtr<UVerticalBox>& Box)
	{
		if (Tree->RootWidget)
		{
			return;
		}
		UOverlay* Root = Tree->ConstructWidget<UOverlay>();
		Tree->RootWidget = Root;
		Highlight = Tree->ConstructWidget<UMvsHighlightBar>();
		UOverlaySlot* BarSlot = Root->AddChildToOverlay(Highlight);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
		Box = Tree->ConstructWidget<UVerticalBox>();
		Root->AddChildToOverlay(Box)->SetHorizontalAlignment(HAlign_Fill);
	}
}

UVerticalBoxSlot* UMvsMenuList::AddItem(UWidget* Item)
{
	EnsureTree(WidgetTree, Highlight, Box);
	Items.Add(Item);
	UVerticalBoxSlot* ItemSlot = Box->AddChildToVerticalBox(Item);
	ItemSlot->SetHorizontalAlignment(HAlign_Fill);
	ItemSlot->SetPadding(FMargin(0.f, 2.f));
	return ItemSlot;
}

TSharedRef<SWidget> UMvsMenuList::RebuildWidget()
{
	EnsureTree(WidgetTree, Highlight, Box);
	return Super::RebuildWidget();
}

void UMvsMenuList::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyTheme(); });
	ApplyTheme();
}

void UMvsMenuList::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UMvsMenuList::ApplyTheme()
{
	const TSharedPtr<SMvsHighlight> Bar = Highlight ? Highlight->GetSlate() : nullptr;
	if (!Bar.IsValid())
	{
		return;
	}
	using namespace MvsStyle;
	const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this);
	const bool bHighContrast = Settings && Settings->GetSettings().bHighContrast;
	FMvsPanelLook Look;
	Look.Corner = 10.f;
	Look.ChamferMask = EMvsChamfer::Opposite;
	Look.Fill = Token(this, EMvsColorToken::Accent, bHighContrast ? 0.3f : 0.14f);
	Look.Edge = Token(this, EMvsColorToken::Accent, bHighContrast ? 1.f : 0.45f);
	Look.EdgeThickness = 1.f;
	Look.Accent = Token(this, EMvsColorToken::Accent);
	Look.AccentWidth = 4.f;
	Look.Glow = Token(this, EMvsColorToken::Accent, 0.14f);
	Look.GlowSize = 6.f;
	Bar->SetLook(Look);
	Bar->SetReducedMotion(MvsMotion::IsReduced(this));
}

void UMvsMenuList::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
	const TSharedPtr<SMvsHighlight> Bar = Highlight ? Highlight->GetSlate() : nullptr;
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
