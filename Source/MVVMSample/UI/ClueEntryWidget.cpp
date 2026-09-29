// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ClueEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "ViewModels/ClueViewModels.h"

TSharedRef<SWidget> UClueEntryWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(FLinearColor(0.03f, 0.03f, 0.04f, 0.8f));
		Frame->SetPadding(FMargin(8.f));
		WidgetTree->RootWidget = Frame;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Frame->SetContent(Row);

		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(72.f);
		Box->SetHeightOverride(72.f);
		Row->AddChildToHorizontalBox(Box)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

		Thumbnail = WidgetTree->ConstructWidget<UImage>();
		Thumbnail->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.18f, 1.f));
		Box->SetContent(Thumbnail);

		UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>();
		Row->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);

		TitleText = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 20;
		TitleText->SetFont(TitleFont);
		Text->AddChildToVerticalBox(TitleText);

		BodyText = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo BodyFont = BodyText->GetFont();
		BodyFont.Size = 14;
		BodyText->SetFont(BodyFont);
		BodyText->SetAutoWrapText(true);
		Text->AddChildToVerticalBox(BodyText);
	}
	return Super::RebuildWidget();
}

void UClueEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	Bind(Cast<UClueEntryViewModel>(ListItemObject));
}

void UClueEntryWidget::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();
	// Scrolled out of view: stop listening and drop any load that would otherwise land on a recycled row.
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
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
		ViewModel->CancelThumbnail();
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UClueEntryViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UClueEntryWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::DisplayTitle, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::DisplayDescription, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsDiscovered, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::Thumbnail, Delegate);
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
	const bool bDiscovered = ViewModel->GetIsDiscovered();
	TitleText->SetText(ViewModel->GetDisplayTitle());
	BodyText->SetText(ViewModel->GetDisplayDescription());
	Frame->SetRenderOpacity(bDiscovered ? 1.f : 0.55f);

	// Thumbnails only load for discovered clues, and only once a row that shows them is on screen.
	if (bDiscovered)
	{
		ViewModel->RequestThumbnail();
	}
	if (UTexture2D* Texture = bDiscovered ? ViewModel->GetThumbnail() : nullptr)
	{
		Thumbnail->SetBrushFromTexture(Texture, true);
		Thumbnail->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		Thumbnail->SetBrush(FSlateBrush());
		Thumbnail->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.18f, 1.f));
	}
}
