// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ObjectiveTrackerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/ComboMeter.h"
#include "UI/Widgets/GothamPanel.h"
#include "ViewModels/ObjectivesViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Objectives"

TSharedRef<SWidget> UObjectiveTrackerWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetMinDesiredWidth(300.f);
		Width->SetMaxDesiredWidth(360.f);
		WidgetTree->RootWidget = Width;

		Panel = WidgetTree->ConstructWidget<UGothamPanel>();
		Panel->SetShape(8.f, EGothamChamfer::TopRight | EGothamChamfer::BottomLeft);
		Panel->SetPanelPadding(FMargin(16.f, 8.f, 14.f, 10.f));
		Width->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		HeaderText = WidgetTree->ConstructWidget<UTextBlock>();
		HeaderText->SetText(LOCTEXT("Objective", "Objective"));
		Column->AddChildToVerticalBox(HeaderText);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
		TitleText = WidgetTree->ConstructWidget<UTextBlock>();
		UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(TitleText);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Bottom);
		ProgressText = WidgetTree->ConstructWidget<UTextBlock>();
		UHorizontalBoxSlot* ProgressSlot = Row->AddChildToHorizontalBox(ProgressText);
		ProgressSlot->SetVerticalAlignment(VAlign_Bottom);
		ProgressSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));

		Bar = WidgetTree->ConstructWidget<UComboMeter>();
		Bar->MeterSize = FVector2D(230.f, 4.f);
		Bar->Skew = 3.f;
		Bar->Gap = 3.f;
		Column->AddChildToVerticalBox(Bar);
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
		LastFound = ViewModel->GetFoundCount();
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

	const bool bComplete = ViewModel->GetIsComplete();
	const FLinearColor Progress = bComplete ? GetToken(EGothamColorToken::Good) : GetToken(EGothamColorToken::Accent);
	FLinearColor Fill = GetToken(EGothamColorToken::Panel);
	Fill.A = GetPanelAlpha() * 0.85f;
	FLinearColor Edge = GetToken(EGothamColorToken::PanelEdge);
	Edge.A = 0.6f;
	Panel->SetColors(Fill, Edge);
	Panel->SetAccent(Progress, 3.f);

	GothamStyle::ApplyText(HeaderText, EGothamTextStyle::Label, GetToken(EGothamColorToken::TextMuted));
	GothamStyle::ApplyText(TitleText, EGothamTextStyle::Header, GetToken(EGothamColorToken::TextPrimary));
	GothamStyle::ApplyText(ProgressText, EGothamTextStyle::Numeric, Progress);
	TitleText->SetText(ViewModel->GetObjectiveTitle());
	ProgressText->SetText(ViewModel->GetProgressText());

	// One segment per clue while that stays readable; a continuous bar beyond that.
	const int32 Total = ViewModel->GetTotalCount();
	FLinearColor Track = GetToken(EGothamColorToken::PanelEdge);
	Track.A = 0.35f;
	Bar->SegmentCount = Total > 0 && Total <= 12 ? Total : 1;
	Bar->FilledColor = Progress;
	Bar->EmptyColor = Track;
	Bar->bReduceMotion = GetGothamSettings().bReducedMotion;
	Bar->SynchronizeProperties();
	Bar->SetPercent(ViewModel->GetProgressPercent());

	if (LastFound >= 0 && ViewModel->GetFoundCount() > LastFound)
	{
		GothamMotion::Pop(ProgressText, 1.3f, 0.2f);
	}
	LastFound = ViewModel->GetFoundCount();
}

#undef LOCTEXT_NAMESPACE
