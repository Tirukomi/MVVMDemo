// Copyright IG. All Rights Reserved.

#include "UI/Screens/ClueLogScreen.h"

#include "UI/Style/MvsMetrics.h"
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
#include "UI/MvsUISettings.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Widgets/MvsPanel.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/MvsViewModelSubsystem.h"

#define LOCTEXT_NAMESPACE "Mvs.ClueLog"

namespace
{
	constexpr int32 TilesPerRow = 4;
}

TSharedRef<SWidget> UClueLogScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Investigation"), LOCTEXT("Title", "Case file"));

		Summary = MakeText(FText::GetEmpty(), EMvsTextStyle::Label, EMvsColorToken::TextMuted);
		Column->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the board. Fixed width for whole tiles (+ scrollbar); height fills the frame.
		USizeBox* BoardWidth = WidgetTree->ConstructWidget<USizeBox>();
		BoardWidth->SetWidthOverride(UClueEntryWidget::TileWidth * TilesPerRow + 16.f);
		Split->AddChildToHorizontalBox(BoardWidth);

		TileView = WidgetTree->ConstructWidget<UMvsClueTileView>();
		TSubclassOf<UUserWidget> EntryClass = UMvsUISettings::Resolve(GetDefault<UMvsUISettings>()->ClueEntryClass);
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
		DetailWidth->SetWidthOverride(MvsMetrics::CaseFileDetailWidth);
		Split->AddChildToHorizontalBox(DetailWidth)->SetVerticalAlignment(VAlign_Top);
		DetailPanel = WidgetTree->ConstructWidget<UMvsPanel>();
		DetailPanel->SetPanelPadding(MvsMetrics::CaseFileDetailPadding);
		DetailPanel->SetShape(14.f, EMvsChamfer::Opposite);
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
		DetailNumber = MakeText(FText::GetEmpty(), EMvsTextStyle::Label, EMvsColorToken::Accent);
		Meta->AddChildToHorizontalBox(DetailNumber)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
		DetailStatus = MakeText(FText::GetEmpty(), EMvsTextStyle::Label, EMvsColorToken::TextMuted);
		Meta->AddChildToHorizontalBox(DetailStatus);

		DetailTitle = MakeText(FText::GetEmpty(), EMvsTextStyle::Header, EMvsColorToken::TextPrimary);
		DetailTitle->SetAutoWrapText(true);
		Detail->AddChildToVerticalBox(DetailTitle)->SetPadding(FMargin(0.f, 4.f, 0.f, 8.f));
		DetailBody = MakeText(FText::GetEmpty(), EMvsTextStyle::Body, EMvsColorToken::TextMuted);
		DetailBody->SetAutoWrapText(true);
		// Long compounds (German) and unbroken runs (pseudo-locale) wrap by character rather than overflow.
		DetailBody->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		Detail->AddChildToVerticalBox(DetailBody);

		AddFooter(MakeActionBar(LOCTEXT("Select", "Browse"), LOCTEXT("Back", "Close")));
	}
	return Super::RebuildWidget();
}

void UClueLogScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>())
		{
			Clues = ViewModels->GetClues();
			using FVM = UClueListViewModel::FFieldNotificationClassDescriptor;
			MvsMVVM::Bind(Clues, this, &UClueLogScreen::OnClueListChanged, { FVM::Entries, FVM::DiscoveredCount });
		}
	}
	RefreshList();
}

void UClueLogScreen::NativeDestruct()
{
	MvsMVVM::Unbind(Clues, this);
	BindDetail(nullptr);
	Super::NativeDestruct();
}

void UClueLogScreen::ApplyTheme(const FMvsTheme& Theme)
{
	Super::ApplyTheme(Theme);
	if (DetailPanel)
	{
		DetailPanel->SetColors(Theme.Color(EMvsColorToken::Panel, Theme.PanelAlpha()),
			Theme.Color(EMvsColorToken::PanelEdge, 0.7f));
		DetailPanel->SetAccent(Theme.Color(EMvsColorToken::Accent), 3.f);
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

	// Only reset the list when the entries themselves changed (which ones, or their order); a discovery just updates
	// the tile in place.
	TArray<UObject*> Items;
	Items.Reserve(Clues->GetEntries().Num());
	for (UClueEntryViewModel* Entry : Clues->GetEntries())
	{
		Items.Add(Entry);
	}
	if (TileView->GetListItems() != Items)
	{
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
	MvsMVVM::Unbind(DetailEntry, this);
	DetailEntry = Entry;
	if (DetailEntry)
	{
		using FVM = UClueEntryViewModel::FFieldNotificationClassDescriptor;
		MvsMVVM::Bind(DetailEntry, this, &UClueLogScreen::OnDetailChanged,
			{ FVM::DisplayTitle, FVM::DisplayDescription, FVM::bIsDiscovered, FVM::Thumbnail });
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
	DetailNumber->SetText(MvsCaseNumber(Clues ? Clues->GetEntries().IndexOfByKey(DetailEntry) : 0));
	DetailStatus->SetText(bDiscovered ? LOCTEXT("Analysed", "Analysed") : LOCTEXT("Unknown", "Not yet found"));
	DetailStatus->SetColorAndOpacity(MvsStyle::Token(this, bDiscovered ? EMvsColorToken::Scanned : EMvsColorToken::TextMuted));
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
		DetailImage->SetColorAndOpacity(MvsStyle::Token(this, EMvsColorToken::PanelEdge, 0.12f));
	}
}

#undef LOCTEXT_NAMESPACE
