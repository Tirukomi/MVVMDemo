// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ComboWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/ComboMeter.h"
#include "UI/Widgets/GothamPanel.h"
#include "UI/Widgets/GothamText.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/GothamMVVM.h"

#define LOCTEXT_NAMESPACE "Gotham.Combo"

TSharedRef<SWidget> UComboWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		WidgetTree->RootWidget = Column;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Row);

		CountText = WidgetTree->ConstructWidget<UGothamText>();
		Row->AddChildToHorizontalBox(CountText)->SetVerticalAlignment(VAlign_Center);

		UVerticalBox* Side = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* SideSlot = Row->AddChildToHorizontalBox(Side);
		SideSlot->SetVerticalAlignment(VAlign_Center);
		SideSlot->SetPadding(FMargin(10.f, 6.f, 0.f, 0.f));

		HitsLabel = WidgetTree->ConstructWidget<UGothamText>();
		HitsLabel->SetText(LOCTEXT("Hits", "Hits"));
		Side->AddChildToVerticalBox(HitsLabel);

		MultiplierTag = WidgetTree->ConstructWidget<UGothamPanel>();
		MultiplierTag->SetShape(5.f, EGothamChamfer::Opposite);
		MultiplierTag->SetPanelPadding(FMargin(8.f, 1.f));
		MultiplierText = WidgetTree->ConstructWidget<UGothamText>();
		MultiplierTag->SetContent(MultiplierText);
		Side->AddChildToVerticalBox(MultiplierTag)->SetHorizontalAlignment(HAlign_Left);

		DecayBar = WidgetTree->ConstructWidget<UComboMeter>();
		DecayBar->SegmentCount = 1;
		DecayBar->MeterSize = FVector2D(170.f, 3.f);
		DecayBar->Gap = 0.f;
		Column->AddChildToVerticalBox(DecayBar)->SetPadding(FMargin(2.f, 2.f, 0.f, 0.f));

		// Milestone callout ("10-HIT COMBO"): shown on each multiple of ten, then fades.
		MilestoneText = WidgetTree->ConstructWidget<UGothamText>();
		MilestoneText->SetRenderOpacity(0.f);
		Column->AddChildToVerticalBox(MilestoneText)->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UComboWidget::SetViewModel(UComboViewModel* InViewModel)
{
	using FVM = UComboViewModel::FFieldNotificationClassDescriptor;
	GothamMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	GothamMVVM::Bind(ViewModel, this, &UComboWidget::OnFieldChanged, { FVM::HitCount, FVM::DecayAlpha, FVM::MultiplierText, FVM::bIsActive });
	GothamMVVM::Bind(ViewModel, this, &UComboWidget::OnMilestone, { FVM::MilestoneCount });
	if (ViewModel)
	{
		LastHits = ViewModel->GetHitCount();
		LastMultiplier = ViewModel->GetMultiplier();
	}
	Refresh();
}

void UComboWidget::NativeDestruct()
{
	FTSTicker::GetCoreTicker().RemoveTicker(MilestoneHandle);
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UComboWidget::OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	// The decay timer changes every frame while a combo is live: touch only the bar.
	if (FieldId == UComboViewModel::FFieldNotificationClassDescriptor::DecayAlpha)
	{
		if (DecayBar && ViewModel)
		{
			DecayBar->SetPercent(ViewModel->GetDecayAlpha());
		}
		return;
	}
	Refresh();
}

void UComboWidget::ApplyStyle()
{
	if (!CountText)
	{
		return;
	}
	const FLinearColor Accent = GetToken(EGothamColorToken::Accent);
	GothamStyle::ApplyText(CountText, EGothamTextStyle::Display, GetToken(EGothamColorToken::TextPrimary));
	GothamStyle::ApplyText(HitsLabel, EGothamTextStyle::Label, GetToken(EGothamColorToken::TextMuted));
	GothamStyle::ApplyText(MultiplierText, EGothamTextStyle::Numeric, GetToken(EGothamColorToken::Panel));
	GothamStyle::ApplyText(MilestoneText, EGothamTextStyle::Header, Accent);
	MultiplierTag->SetColors(Accent, FLinearColor::Transparent, 0.f);

	FLinearColor Track = GetToken(EGothamColorToken::PanelEdge);
	Track.A = 0.3f;
	DecayBar->FilledColor = Accent;
	DecayBar->EmptyColor = Track;
	DecayBar->bReduceMotion = GetGothamSettings().bReducedMotion;
	DecayBar->SynchronizeProperties();
}

void UComboWidget::Refresh()
{
	if (!ViewModel || !CountText)
	{
		return;
	}
	SetVisibility(ViewModel->GetIsActive() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	CountText->SetText(FText::AsNumber(ViewModel->GetHitCount()));
	MultiplierText->SetText(ViewModel->GetMultiplierText());
	DecayBar->SetPercent(ViewModel->GetDecayAlpha());

	if (ViewModel->GetHitCount() > LastHits)
	{
		GothamMotion::Pop(CountText, 1.22f);
	}
	if (ViewModel->GetMultiplier() > LastMultiplier)
	{
		GothamMotion::Pop(MultiplierTag, 1.35f, 0.2f);
	}
	LastHits = ViewModel->GetHitCount();
	LastMultiplier = ViewModel->GetMultiplier();
}

void UComboWidget::OnMilestone(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	if (!ViewModel || !MilestoneText)
	{
		return;
	}
	// Text first, then a pop; hold, then fade. Under reduced motion there is no pop and the fade is a cut.
	MilestoneText->SetText(ViewModel->GetMilestoneText());
	MilestoneText->SetRenderOpacity(1.f);
	GothamMotion::Pop(MilestoneText, 1.3f, 0.22f);
	FTSTicker::GetCoreTicker().RemoveTicker(MilestoneHandle);
	MilestoneHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		GothamMotion::Fade(MilestoneText, 1.f, 0.f, 0.45f);
		MilestoneHandle.Reset();
		return false;
	}), MilestoneHoldSeconds);
}

#undef LOCTEXT_NAMESPACE
