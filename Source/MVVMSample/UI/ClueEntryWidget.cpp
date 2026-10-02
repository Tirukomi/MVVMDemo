// Copyright IG. All Rights Reserved.

#include "UI/ClueEntryWidget.h"
#include "UI/Style/MvsStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "UI/Slate/SClueTile.h"
#include "Widgets/Text/STextBlock.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsMVVM.h"

#define LOCTEXT_NAMESPACE "Mvs.ClueLog"

FText MvsCaseNumber(int32 Index)
{
	FNumberFormattingOptions Digits;
	Digits.MinimumIntegralDigits = 3;
	Digits.UseGrouping = false;
	return FText::Format(LOCTEXT("CaseNumberFmt", "No. {0}"), FText::AsNumber(Index + 1, &Digits));
}

TSharedRef<SWidget> UMvsClueTile::RebuildWidget()
{
	Tile = SNew(SClueTile);
	return Tile.ToSharedRef();
}

void UMvsClueTile::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Tile.Reset();
}

TSharedRef<SWidget> UClueEntryWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// Leave a gutter inside the tile slot so neighbours' glows do not touch.
		USizeBox* Outer = WidgetTree->ConstructWidget<USizeBox>();
		Outer->SetWidthOverride(TileWidth - 12.f);
		Outer->SetHeightOverride(TileHeight - 12.f);
		// The list row takes hover, clicks and selection; nothing inside the tile is a hit-test target. The tile view
		// re-adds every visible tile on each frame of a scroll, so each hit-testable widget was churn (S7).
		Outer->SetVisibility(ESlateVisibility::HitTestInvisible);
		WidgetTree->RootWidget = Outer;

		// One widget for the frame, thumbnail and texts (SClueTile): it used to be six containers and three texts.
		Tile = WidgetTree->ConstructWidget<UMvsClueTile>();
		Outer->SetContent(Tile);
	}
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	if (SClueTile* Slate = Tile ? Tile->GetTile() : nullptr)
	{
		Slate->GetMark().SetText(FText::FromString(TEXT("?")));
	}
	Refresh();
	return Built;
}

void UClueEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	Bind(Cast<UClueEntryViewModel>(ListItemObject));
}

void UClueEntryWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);
	bSelected = bIsSelected;
	ApplySelection(MvsStyle::Theme(this));
}

void UClueEntryWidget::NativeConstruct()
{
	// Deliberately NOT MvsUI::DisableTick: list views force their entry rows to tick after generating them
	// (UListViewBase::HandleGenerateRow calls SetCanTick(true) so selection works). An entry flagged Never would then
	// trip UUserWidget::NativeTick's "mismatching tick states" ensure. The tick itself is empty and bounded by the
	// tiles on screen.
	Super::NativeConstruct();
}

void UClueEntryWidget::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();
	// Scrolled out of view: stop listening and drop any load that would otherwise land on a recycled tile.
	Bind(nullptr);
}

void UClueEntryWidget::NativeDestruct()
{
	Bind(nullptr);
	Super::NativeDestruct();
}

void UClueEntryWidget::Bind(UClueEntryViewModel* InViewModel)
{
	if (ViewModel)
	{
		MvsMVVM::Unbind(ViewModel, this);
		ViewModel->CancelThumbnail();
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UClueEntryViewModel::FFieldNotificationClassDescriptor;
		MvsMVVM::Bind(ViewModel, this, &UClueEntryWidget::OnFieldChanged, { FVM::DisplayTitle, FVM::bIsDiscovered, FVM::Thumbnail });
		if (ViewModel->GetIsDiscovered())
		{
			ViewModel->RequestThumbnail();
		}
	}
	Refresh();
}

void UClueEntryWidget::Refresh()
{
	SClueTile* Slate = Tile ? Tile->GetTile() : nullptr;
	if (!Slate || !ViewModel)
	{
		return;
	}
	// One theme per refresh (rebinding while scrolling runs this several times a frame), not one per colour.
	const FMvsTheme Theme = MvsStyle::Theme(this);
	const bool bDiscovered = ViewModel->GetIsDiscovered();

	// The title is a Label: capitals, as UMvsText shows that style (culture-aware, so only when it changed).
	if (!ShownTitle.IdenticalTo(ViewModel->GetDisplayTitle()))
	{
		ShownTitle = ViewModel->GetDisplayTitle();
		Slate->GetTitle().SetText(ShownTitle.ToUpper());
	}
	Slate->GetTitle().SetFont(Theme.Font(EMvsTextStyle::Label));
	Slate->GetTitle().SetColorAndOpacity(Theme.Color(bDiscovered ? EMvsColorToken::TextPrimary : EMvsColorToken::TextMuted));

	// The entry knows its place in the list; asking the list searched all of it on every rebind.
	const int32 Index = ViewModel->GetListIndex();
	Slate->GetNumber().SetText(Index != INDEX_NONE ? MvsCaseNumber(Index) : FText::GetEmpty());
	Slate->GetNumber().SetFont(Theme.Font(EMvsTextStyle::Key));
	Slate->GetNumber().SetColorAndOpacity(Theme.Color(EMvsColorToken::TextMuted));

	// Thumbnails only load for discovered clues, and only once a tile that shows them is on screen.
	if (bDiscovered)
	{
		ViewModel->RequestThumbnail();
	}
	UTexture2D* Texture = bDiscovered ? ViewModel->GetThumbnail() : nullptr;
	if (Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
		Slate->SetThumbnail(Brush, FLinearColor::White);
	}
	else
	{
		Slate->SetThumbnail(FSlateBrush(), Theme.Color(EMvsColorToken::PanelEdge, 0.12f));
	}
	Slate->GetMark().SetVisibility(bDiscovered ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	Slate->GetMark().SetFont(Theme.Font(EMvsTextStyle::Display));
	Slate->GetMark().SetColorAndOpacity(Theme.Color(EMvsColorToken::PanelEdge, 0.5f));
	ApplySelection(Theme);
}

void UClueEntryWidget::ApplySelection(const FMvsTheme& Theme)
{
	SClueTile* Slate = Tile ? Tile->GetTile() : nullptr;
	if (!Slate)
	{
		return;
	}
	const FLinearColor Accent = Theme.Color(EMvsColorToken::Accent);
	FMvsPanelLook Look;
	Look.Corner = 10.f;
	Look.ChamferMask = EMvsChamfer::Opposite;
	Look.Fill = Theme.Color(EMvsColorToken::Panel, Theme.PanelAlpha());
	Look.Edge = bSelected ? Accent : Theme.Color(EMvsColorToken::PanelEdge, 0.45f);
	Look.EdgeThickness = bSelected ? 1.5f : 1.f;
	Look.Accent = Accent;
	Look.AccentWidth = bSelected ? 3.f : 0.f;
	Look.Glow = Theme.Color(EMvsColorToken::Accent, 0.3f);
	Look.GlowSize = bSelected ? 5.f : 0.f;
	Slate->SetLook(Look);
}

#undef LOCTEXT_NAMESPACE
