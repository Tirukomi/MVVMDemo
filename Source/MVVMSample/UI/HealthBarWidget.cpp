// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/HealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "ViewModels/PlayerVitalsViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.HealthBar"

TSharedRef<SWidget> UHealthBarWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(360.f);
		Size->SetHeightOverride(28.f);
		WidgetTree->RootWidget = Size;

		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
		Size->SetContent(Overlay);

		Bar = WidgetTree->ConstructWidget<UProgressBar>();
		UOverlaySlot* BarSlot = Overlay->AddChildToOverlay(Bar);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);

		Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 14;
		Label->SetFont(Font);
		UOverlaySlot* LabelSlot = Overlay->AddChildToOverlay(Label);
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Super::RebuildWidget();
}

void UHealthBarWidget::SetViewModel(UPlayerVitalsViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UPlayerVitalsViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UHealthBarWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::HealthPercent, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::Health, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsLowHealth, Delegate);
	}
	Refresh();
}

void UHealthBarWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UHealthBarWidget::Refresh()
{
	if (!Bar || !Label || !ViewModel)
	{
		return;
	}
	Bar->SetPercent(ViewModel->GetHealthPercent());
	const bool bLow = ViewModel->GetIsLowHealth();
	Bar->SetFillColorAndOpacity(GetToken(bLow ? EGothamColorToken::Danger : EGothamColorToken::Good));
	// Low health is also spelled out in text, so the warning never depends on colour alone.
	Label->SetText(FText::Format(bLow ? LOCTEXT("LowHealthFmt", "LOW  {0} / {1}") : LOCTEXT("HealthFmt", "{0} / {1}"),
		FText::AsNumber(FMath::CeilToInt(ViewModel->GetHealth())), FText::AsNumber(FMath::RoundToInt(ViewModel->GetMaxHealth()))));
}

#undef LOCTEXT_NAMESPACE
