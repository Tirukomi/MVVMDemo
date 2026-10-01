// Copyright IG. All Rights Reserved.

#include "UI/ObjectiveTrackerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/ComboMeter.h"
#include "UI/Widgets/MvsPanel.h"
#include "UI/Widgets/MvsText.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/ObjectivesViewModel.h"

#define LOCTEXT_NAMESPACE "Mvs.Objectives"

TSharedRef<SWidget> UObjectiveTrackerWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetMinDesiredWidth(300.f);
		Width->SetMaxDesiredWidth(360.f);
		WidgetTree->RootWidget = Width;

		Panel = WidgetTree->ConstructWidget<UMvsPanel>();
		Panel->SetShape(8.f, EMvsChamfer::TopRight | EMvsChamfer::BottomLeft);
		Panel->SetPanelPadding(FMargin(16.f, 8.f, 14.f, 10.f));
		Width->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		HeaderText = WidgetTree->ConstructWidget<UMvsText>();
		HeaderText->SetText(LOCTEXT("Objective", "Objective"));
		Column->AddChildToVerticalBox(HeaderText);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
		TitleText = WidgetTree->ConstructWidget<UMvsText>();
		UHorizontalBoxSlot* TitleSlot = Row->AddChildToHorizontalBox(TitleText);
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Bottom);
		ProgressText = WidgetTree->ConstructWidget<UMvsText>();
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
	using FVM = UObjectivesViewModel::FFieldNotificationClassDescriptor;
	MvsMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	MvsMVVM::Bind(ViewModel, this, &UObjectiveTrackerWidget::OnFieldChanged,
		{ FVM::ObjectiveTitle, FVM::ProgressText, FVM::ProgressPercent, FVM::bIsComplete });
	if (ViewModel)
	{
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
	const FLinearColor Progress = bComplete ? GetToken(EMvsColorToken::Good) : GetToken(EMvsColorToken::Accent);
	FLinearColor Fill = GetToken(EMvsColorToken::Panel);
	Fill.A = GetPanelAlpha() * 0.85f;
	FLinearColor Edge = GetToken(EMvsColorToken::PanelEdge);
	Edge.A = 0.6f;
	Panel->SetColors(Fill, Edge);
	Panel->SetAccent(Progress, 3.f);

	MvsStyle::ApplyText(HeaderText, EMvsTextStyle::Label, GetToken(EMvsColorToken::TextMuted));
	MvsStyle::ApplyText(TitleText, EMvsTextStyle::Header, GetToken(EMvsColorToken::TextPrimary));
	MvsStyle::ApplyText(ProgressText, EMvsTextStyle::Numeric, Progress);
	TitleText->SetText(ViewModel->GetObjectiveTitle());
	ProgressText->SetText(ViewModel->GetProgressText());

	// One segment per clue while that stays readable; a continuous bar beyond that.
	const int32 Total = ViewModel->GetTotalCount();
	FLinearColor Track = GetToken(EMvsColorToken::PanelEdge);
	Track.A = 0.35f;
	Bar->SegmentCount = Total > 0 && Total <= 12 ? Total : 1;
	Bar->FilledColor = Progress;
	Bar->EmptyColor = Track;
	Bar->bReduceMotion = GetMvsSettings().bReducedMotion;
	Bar->SynchronizeProperties();
	Bar->SetPercent(ViewModel->GetProgressPercent());

	if (LastFound >= 0 && ViewModel->GetFoundCount() > LastFound)
	{
		MvsMotion::Pop(ProgressText, 1.3f, 0.2f);
	}
	LastFound = ViewModel->GetFoundCount();
}

#undef LOCTEXT_NAMESPACE
