// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ComboWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ViewModels/ComboViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Combo"

TSharedRef<SWidget> UComboWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(180.f);
		WidgetTree->RootWidget = Size;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Size->SetContent(Column);

		auto AddText = [&](int32 FontSize)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = FontSize;
			Text->SetFont(Font);
			Text->SetJustification(ETextJustify::Right);
			Column->AddChildToVerticalBox(Text)->SetHorizontalAlignment(HAlign_Right);
			return Text;
		};

		MultiplierText = AddText(40);
		HitsText = AddText(16);

		DecayBar = WidgetTree->ConstructWidget<UProgressBar>();
		DecayBar->SetFillColorAndOpacity(FLinearColor(0.95f, 0.75f, 0.2f));
		Column->AddChildToVerticalBox(DecayBar)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UComboWidget::SetViewModel(UComboViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UComboViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UComboWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::HitCount, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::DecayAlpha, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::MultiplierText, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsActive, Delegate);
	}
	Refresh();
}

void UComboWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UComboWidget::Refresh()
{
	if (!ViewModel || !HitsText)
	{
		return;
	}
	SetVisibility(ViewModel->GetIsActive() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	MultiplierText->SetText(ViewModel->GetMultiplierText());
	HitsText->SetText(FText::Format(LOCTEXT("HitsFmt", "{0} hits"), FText::AsNumber(ViewModel->GetHitCount())));
	DecayBar->SetPercent(ViewModel->GetDecayAlpha());
}

#undef LOCTEXT_NAMESPACE
