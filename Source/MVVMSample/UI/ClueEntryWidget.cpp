// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ClueEntryWidget.h"
#include "UI/Style/GothamStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamPanel.h"
#include "UI/Widgets/GothamText.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamMVVM.h"

#define LOCTEXT_NAMESPACE "Gotham.ClueLog"

FText GothamCaseNumber(int32 Index)
{
	FNumberFormattingOptions Digits;
	Digits.MinimumIntegralDigits = 3;
	Digits.UseGrouping = false;
	return FText::Format(LOCTEXT("CaseNumberFmt", "No. {0}"), FText::AsNumber(Index + 1, &Digits));
}

TSharedRef<SWidget> UClueEntryWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// Leave a gutter inside the tile slot so neighbours' glows do not touch.
		USizeBox* Outer = WidgetTree->ConstructWidget<USizeBox>();
		Outer->SetWidthOverride(TileWidth - 12.f);
		Outer->SetHeightOverride(TileHeight - 12.f);
		WidgetTree->RootWidget = Outer;

		Frame = WidgetTree->ConstructWidget<UGothamPanel>();
		Frame->SetPanelPadding(FMargin(7.f));
		Frame->SetShape(10.f, EGothamChamfer::Opposite);
		Outer->SetContent(Frame);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Frame->SetContent(Column);

		UOverlay* Picture = WidgetTree->ConstructWidget<UOverlay>();
		UVerticalBoxSlot* PictureSlot = Column->AddChildToVerticalBox(Picture);
		PictureSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		Thumbnail = WidgetTree->ConstructWidget<UImage>();
		UOverlaySlot* ThumbSlot = Picture->AddChildToOverlay(Thumbnail);
		ThumbSlot->SetHorizontalAlignment(HAlign_Fill);
		ThumbSlot->SetVerticalAlignment(VAlign_Fill);

		UnknownMark = WidgetTree->ConstructWidget<UGothamText>();
		UnknownMark->SetText(FText::FromString(TEXT("?")));
		UnknownMark->SetFont(GothamStyle::Font(EGothamTextStyle::Display));
		UOverlaySlot* MarkSlot = Picture->AddChildToOverlay(UnknownMark);
		MarkSlot->SetHorizontalAlignment(HAlign_Center);
		MarkSlot->SetVerticalAlignment(VAlign_Center);

		CaseNumber = WidgetTree->ConstructWidget<UGothamText>();
		CaseNumber->SetFont(GothamStyle::Font(EGothamTextStyle::Key));
		UOverlaySlot* NumberSlot = Picture->AddChildToOverlay(CaseNumber);
		NumberSlot->SetPadding(FMargin(5.f, 3.f));

		TitleText = WidgetTree->ConstructWidget<UGothamText>();
		GothamStyle::ApplyText(TitleText, EGothamTextStyle::Label, FLinearColor::White);
		TitleText->SetClipping(EWidgetClipping::ClipToBounds);
		Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(1.f, 6.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
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
	ApplySelection();
}

void UClueEntryWidget::NativeConstruct()
{
	// Deliberately NOT GothamUI::DisableTick: list views force their entry rows to tick after generating them
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
		GothamMVVM::Unbind(ViewModel, this);
		ViewModel->CancelThumbnail();
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UClueEntryViewModel::FFieldNotificationClassDescriptor;
		GothamMVVM::Bind(ViewModel, this, &UClueEntryWidget::OnFieldChanged, { FVM::DisplayTitle, FVM::bIsDiscovered, FVM::Thumbnail });
		if (ViewModel->GetIsDiscovered())
		{
			ViewModel->RequestThumbnail();
		}
	}
	Refresh();
}

void UClueEntryWidget::Refresh()
{
	if (!Frame || !ViewModel)
	{
		return;
	}
	using namespace GothamStyle;
	const bool bDiscovered = ViewModel->GetIsDiscovered();
	TitleText->SetText(ViewModel->GetDisplayTitle());
	TitleText->SetColorAndOpacity(Token(this, bDiscovered ? EGothamColorToken::TextPrimary : EGothamColorToken::TextMuted));

	const UListView* Owner = Cast<UListView>(GetOwningListView());
	CaseNumber->SetText(Owner ? GothamCaseNumber(Owner->GetIndexForItem(ViewModel)) : FText::GetEmpty());
	CaseNumber->SetColorAndOpacity(Token(this, EGothamColorToken::TextMuted));

	// Thumbnails only load for discovered clues, and only once a tile that shows them is on screen.
	if (bDiscovered)
	{
		ViewModel->RequestThumbnail();
	}
	UTexture2D* Texture = bDiscovered ? ViewModel->GetThumbnail() : nullptr;
	if (Texture)
	{
		Thumbnail->SetBrushFromTexture(Texture, false);
		Thumbnail->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		Thumbnail->SetBrush(FSlateBrush());
		Thumbnail->SetColorAndOpacity(Token(this, EGothamColorToken::PanelEdge, 0.12f));
	}
	UnknownMark->SetVisibility(bDiscovered ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	UnknownMark->SetColorAndOpacity(Token(this, EGothamColorToken::PanelEdge, 0.5f));
	ApplySelection();
}

void UClueEntryWidget::ApplySelection()
{
	if (!Frame)
	{
		return;
	}
	using namespace GothamStyle;
	const FLinearColor Accent = Token(this, EGothamColorToken::Accent);
	Frame->SetColors(Token(this, EGothamColorToken::Panel, PanelAlpha(this)),
		bSelected ? Accent : Token(this, EGothamColorToken::PanelEdge, 0.45f), bSelected ? 1.5f : 1.f);
	Frame->SetAccent(Accent, bSelected ? 3.f : 0.f);
	Frame->SetGlow(Token(this, EGothamColorToken::Accent, 0.3f), bSelected ? 5.f : 0.f);
}

#undef LOCTEXT_NAMESPACE
