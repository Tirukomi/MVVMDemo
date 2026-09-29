// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ObjectiveTrackerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ViewModels/ObjectivesViewModel.h"

TSharedRef<SWidget> UObjectiveTrackerWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(240.f);
		WidgetTree->RootWidget = Size;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Size->SetContent(Column);

		auto AddText = [&](int32 FontSize)
		{
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = FontSize;
			Text->SetFont(Font);
			Column->AddChildToVerticalBox(Text);
			return Text;
		};
		TitleText = AddText(18);
		ProgressText = AddText(26);

		Bar = WidgetTree->ConstructWidget<UProgressBar>();
		Bar->SetFillColorAndOpacity(FLinearColor(0.4f, 0.85f, 1.f));
		Column->AddChildToVerticalBox(Bar)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UObjectiveTrackerWidget::SetViewModel(UObjectivesViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UObjectivesViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UObjectiveTrackerWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::ObjectiveTitle, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::ProgressText, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::ProgressPercent, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsComplete, Delegate);
	}
	Refresh();
}

void UObjectiveTrackerWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UObjectiveTrackerWidget::Refresh()
{
	if (!ViewModel || !TitleText)
	{
		return;
	}
	// Hidden until the level actually has clues to find.
	SetVisibility(ViewModel->GetTotalCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	TitleText->SetText(ViewModel->GetObjectiveTitle());
	ProgressText->SetText(ViewModel->GetProgressText());
	Bar->SetPercent(ViewModel->GetProgressPercent());
	Bar->SetFillColorAndOpacity(ViewModel->GetIsComplete() ? FLinearColor(0.3f, 0.9f, 0.4f) : FLinearColor(0.4f, 0.85f, 1.f));
}
