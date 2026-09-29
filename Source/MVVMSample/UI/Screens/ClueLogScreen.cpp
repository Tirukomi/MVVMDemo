// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/ClueLogScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/ClueEntryWidget.h"
#include "UI/GothamUISettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"

#define LOCTEXT_NAMESPACE "Gotham.ClueLog"

TSharedRef<SWidget> UClueLogScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Dim->SetContent(Column);

		Column->AddChildToVerticalBox(MakeTitle(LOCTEXT("Title", "CASE FILE")));

		Summary = WidgetTree->ConstructWidget<UTextBlock>();
		Summary->SetJustification(ETextJustify::Center);
		Column->AddChildToVerticalBox(Summary)->SetPadding(FMargin(0.f, 4.f, 0.f, 16.f));

		USizeBox* ListBox = WidgetTree->ConstructWidget<USizeBox>();
		ListBox->SetWidthOverride(720.f);
		ListBox->SetHeightOverride(400.f);
		Column->AddChildToVerticalBox(ListBox);

		ListView = WidgetTree->ConstructWidget<UGothamClueListView>();
		TSubclassOf<UUserWidget> EntryClass = GetDefault<UGothamUISettings>()->ClueEntryClass.LoadSynchronous();
		ListView->SetEntryClass(EntryClass ? EntryClass : TSubclassOf<UUserWidget>(UClueEntryWidget::StaticClass()));
		ListBox->SetContent(ListView);
		DefaultFocus = ListView;

		Column->AddChildToVerticalBox(MakeHintBar(LOCTEXT("Select", "Select"), LOCTEXT("Back", "Close")))
			->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
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
	Super::NativeDestruct();
}

void UClueLogScreen::RefreshList()
{
	if (!Clues || !ListView)
	{
		return;
	}
	Summary->SetText(FText::Format(LOCTEXT("SummaryFmt", "{0} of {1} clues discovered"),
		FText::AsNumber(Clues->GetDiscoveredCount()), FText::AsNumber(Clues->GetTotalCount())));

	// Only reset the list when the set of entries changed; a discovery just updates the row in place.
	if (ListView->GetNumItems() != Clues->GetEntries().Num())
	{
		TArray<UObject*> Items;
		Items.Reserve(Clues->GetEntries().Num());
		for (UClueEntryViewModel* Entry : Clues->GetEntries())
		{
			Items.Add(Entry);
		}
		ListView->SetListItems(Items);
	}
}

#undef LOCTEXT_NAMESPACE
