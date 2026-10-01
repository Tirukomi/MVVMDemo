// Copyright IG. All Rights Reserved.

#include "UI/HealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/ComboMeter.h"
#include "UI/Widgets/MvsText.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/PlayerVitalsViewModel.h"

#define LOCTEXT_NAMESPACE "Mvs.HealthBar"

namespace
{
	constexpr int32 HealthSegments = 10;
}

TSharedRef<SWidget> UHealthBarWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		WidgetTree->RootWidget = Column;

		Bar = WidgetTree->ConstructWidget<UComboMeter>();
		Bar->SegmentCount = HealthSegments;
		Bar->MeterSize = FVector2D(380.f, 16.f);
		Bar->Skew = 8.f;
		Bar->Gap = 4.f;
		Column->AddChildToVerticalBox(Bar);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

		StatusText = WidgetTree->ConstructWidget<UMvsText>();
		Row->AddChildToHorizontalBox(StatusText)->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

		ValueText = WidgetTree->ConstructWidget<UMvsText>();
		Row->AddChildToHorizontalBox(ValueText);
	}
	return Super::RebuildWidget();
}

void UHealthBarWidget::SetViewModel(UPlayerVitalsViewModel* InViewModel)
{
	using FVM = UPlayerVitalsViewModel::FFieldNotificationClassDescriptor;
	MvsMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	MvsMVVM::Bind(ViewModel, this, &UHealthBarWidget::OnFieldChanged, { FVM::HealthPercent, FVM::Health, FVM::bIsLowHealth });
	Refresh();
}

void UHealthBarWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UHealthBarWidget::Refresh()
{
	if (!Bar || !ViewModel)
	{
		return;
	}
	const bool bLow = ViewModel->GetIsLowHealth();
	FLinearColor Empty = GetToken(EMvsColorToken::PanelEdge);
	Empty.A = 0.35f;
	FLinearColor Ghost = GetToken(EMvsColorToken::Danger);
	Ghost.A = 0.8f;

	Bar->FilledColor = bLow ? GetToken(EMvsColorToken::Danger) : GetToken(EMvsColorToken::TextPrimary);
	Bar->EmptyColor = Empty;
	Bar->GhostColor = Ghost;
	Bar->bReduceMotion = GetMvsSettings().bReducedMotion;
	Bar->SynchronizeProperties();
	Bar->SetPercent(ViewModel->GetHealthPercent());

	MvsStyle::ApplyText(StatusText, EMvsTextStyle::Label, GetToken(EMvsColorToken::Danger));
	StatusText->SetText(LOCTEXT("Low", "Low"));
	StatusText->SetVisibility(bLow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	MvsStyle::ApplyText(ValueText, EMvsTextStyle::Label, GetToken(EMvsColorToken::TextMuted));
	ValueText->SetText(FText::Format(LOCTEXT("HealthFmt", "{0} / {1}"),
		FText::AsNumber(FMath::CeilToInt(ViewModel->GetHealth())), FText::AsNumber(FMath::RoundToInt(ViewModel->GetMaxHealth()))));
}

#undef LOCTEXT_NAMESPACE
