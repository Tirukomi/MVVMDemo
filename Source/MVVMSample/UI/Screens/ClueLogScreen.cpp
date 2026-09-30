// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/ClueLogScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "UI/ClueEntryWidget.h"
#include "UI/GothamUISettings.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamPanel.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"

#define LOCTEXT_NAMESPACE "Gotham.ClueLog"

namespace
{
	constexpr int32 TilesPerRow = 4;
}

TSharedRef<SWidget> UClueLogScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// The key that opened this screen (ClueLog) closes it again.
		ToggleActionName = TEXT("ClueLog");
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Investigation"), LOCTEXT("Title", "Case file"));

		Summary = MakeText(FText::GetEmpty(), EGothamTextStyle::Label, EGothamColorToken::TextMuted);
		Column->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the board. Fixed width for whole tiles (+ scrollbar); height fills the frame.
		USizeBox* BoardWidth = WidgetTree->ConstructWidget<USizeBox>();
		BoardWidth->SetWidthOverride(UClueEntryWidget::TileWidth * TilesPerRow + 16.f);
		Split->AddChildToHorizontalBox(BoardWidth);

		TileView = WidgetTree->ConstructWidget<UGothamClueTileView>();
		TSubclassOf<UUserWidget> EntryClass = GetDefault<UGothamUISettings>()->ClueEntryClass.LoadSynchronous();
		TileView->SetEntryClass(EntryClass ? EntryClass : TSubclassOf<UUserWidget>(UClueEntryWidget::StaticClass()));
		TileView->SetEntryWidth(UClueEntryWidget::TileWidth);
		TileView->SetEntryHeight(UClueEntryWidget::TileHeight);
		TileView->SetSelectionMode(ESelectionMode::Single);
		TileView->OnItemSelectionChanged().AddUObject(this, &UClueLogScreen::OnSelectionChanged);
		BoardWidth->SetContent(TileView);
		DefaultFocus = TileView;

		Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Right: the selected clue.
		USizeBox* DetailWidth = WidgetTree->ConstructWidget<USizeBox>();
		DetailWidth->SetWidthOverride(420.f);
		Split->AddChildToHorizontalBox(DetailWidth)->SetVerticalAlignment(VAlign_Top);
		DetailPanel = WidgetTree->ConstructWidget<UGothamPanel>();
		DetailPanel->SetPanelPadding(FMargin(18.f, 18.f, 18.f, 20.f));
		DetailPanel->SetShape(14.f, EGothamChamfer::Opposite);
		DetailWidth->SetContent(DetailPanel);

		UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailPanel->SetContent(Detail);
		USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>();
		ImageBox->SetHeightOverride(228.f);
		Detail->AddChildToVerticalBox(ImageBox)->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		DetailImage = WidgetTree->ConstructWidget<UImage>();
		ImageBox->SetContent(DetailImage);

		UHorizontalBox* Meta = WidgetTree->ConstructWidget<UHorizontalBox>();
		Detail->AddChildToVerticalBox(Meta);
		DetailNumber = MakeText(FText::GetEmpty(), EGothamTextStyle::Label, EGothamColorToken::Accent);
		Meta->AddChildToHorizontalBox(DetailNumber)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
		DetailStatus = MakeText(FText::GetEmpty(), EGothamTextStyle::Label, EGothamColorToken::TextMuted);
		Meta->AddChildToHorizontalBox(DetailStatus);

		DetailTitle = MakeText(FText::GetEmpty(), EGothamTextStyle::Header, EGothamColorToken::TextPrimary);
		DetailTitle->SetAutoWrapText(true);
		Detail->AddChildToVerticalBox(DetailTitle)->SetPadding(FMargin(0.f, 4.f, 0.f, 8.f));
		DetailBody = MakeText(FText::GetEmpty(), EGothamTextStyle::Body, EGothamColorToken::TextMuted);
		DetailBody->SetAutoWrapText(true);
		// Long compounds (German) and unbroken runs (pseudo-locale) wrap by character rather than overflow.
		DetailBody->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		Detail->AddChildToVerticalBox(DetailBody);

		AddFooter(MakeHintBar(LOCTEXT("Select", "Browse"), LOCTEXT("Back", "Close")));
	}
	return Super::RebuildWidget();
}

void UClueLogScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>())
		{
			Clues = ViewModels->GetClues();
			using FVM = UClueListViewModel::FFieldNotificationClassDescriptor;
			const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UClueLogScreen::OnClueListChanged);
			Clues->AddFieldValueChangedDelegate(FVM::Entries, Delegate);
			Clues->AddFieldValueChangedDelegate(FVM::DiscoveredCount, Delegate);
		}
	}
	RefreshList();
}

void UClueLogScreen::NativeDestruct()
{
	if (Clues)
	{
		Clues->RemoveAllFieldValueChangedDelegates(this);
	}
	BindDetail(nullptr);
	Super::NativeDestruct();
}

void UClueLogScreen::OnPaletteChanged()
{
	Super::OnPaletteChanged();
	if (DetailPanel)
	{
		DetailPanel->SetColors(GothamStyle::Token(this, EGothamColorToken::Panel, GothamStyle::PanelAlpha(this)),
			GothamStyle::Token(this, EGothamColorToken::PanelEdge, 0.7f));
		DetailPanel->SetAccent(GothamStyle::Token(this, EGothamColorToken::Accent), 3.f);
	}
	RefreshDetail();
}

void UClueLogScreen::RefreshList()
{
	if (!Clues || !TileView)
	{
		return;
	}
	Summary->SetText(FText::Format(LOCTEXT("SummaryFmt", "{0} of {1} clues discovered"),
		FText::AsNumber(Clues->GetDiscoveredCount()), FText::AsNumber(Clues->GetTotalCount())));

	// Only reset the list when the set of entries changed; a discovery just updates the tile in place.
	if (TileView->GetNumItems() != Clues->GetEntries().Num())
	{
		TArray<UObject*> Items;
		Items.Reserve(Clues->GetEntries().Num());
		for (UClueEntryViewModel* Entry : Clues->GetEntries())
		{
			Items.Add(Entry);
		}
		TileView->SetListItems(Items);
		if (!Items.IsEmpty())
		{
			TileView->SetSelectedIndex(0);
		}
	}
}

void UClueLogScreen::OnSelectionChanged(UObject* Item)
{
	BindDetail(Cast<UClueEntryViewModel>(Item));
}

void UClueLogScreen::BindDetail(UClueEntryViewModel* Entry)
{
	if (DetailEntry)
	{
		DetailEntry->RemoveAllFieldValueChangedDelegates(this);
	}
	DetailEntry = Entry;
	if (DetailEntry)
	{
		using FVM = UClueEntryViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UClueLogScreen::OnDetailChanged);
		DetailEntry->AddFieldValueChangedDelegate(FVM::DisplayTitle, Delegate);
		DetailEntry->AddFieldValueChangedDelegate(FVM::DisplayDescription, Delegate);
		DetailEntry->AddFieldValueChangedDelegate(FVM::bIsDiscovered, Delegate);
		DetailEntry->AddFieldValueChangedDelegate(FVM::Thumbnail, Delegate);
		if (DetailEntry->GetIsDiscovered())
		{
			DetailEntry->RequestThumbnail();
		}
	}
	RefreshDetail();
}

void UClueLogScreen::RefreshDetail()
{
	if (!DetailTitle)
	{
		return;
	}
	if (!DetailEntry)
	{
		DetailPanel->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const bool bDiscovered = DetailEntry->GetIsDiscovered();
	DetailNumber->SetText(GothamCaseNumber(Clues ? Clues->GetEntries().IndexOfByKey(DetailEntry) : 0));
	DetailStatus->SetText(bDiscovered ? LOCTEXT("Analysed", "Analysed") : LOCTEXT("Unknown", "Not yet found"));
	DetailStatus->SetColorAndOpacity(GothamStyle::Token(this, bDiscovered ? EGothamColorToken::Scanned : EGothamColorToken::TextMuted));
	DetailTitle->SetText(DetailEntry->GetDisplayTitle());
	DetailBody->SetText(DetailEntry->GetDisplayDescription());

	if (UTexture2D* Texture = bDiscovered ? DetailEntry->GetThumbnail() : nullptr)
	{
		DetailImage->SetBrushFromTexture(Texture, false);
		DetailImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		DetailImage->SetBrush(FSlateBrush());
		DetailImage->SetColorAndOpacity(GothamStyle::Token(this, EGothamColorToken::PanelEdge, 0.12f));
	}
}

#undef LOCTEXT_NAMESPACE
