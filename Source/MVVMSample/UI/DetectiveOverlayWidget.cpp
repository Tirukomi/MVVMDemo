// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/DetectiveOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/GothamUISettings.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "ViewModels/DetectiveViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.DetectiveOverlay"

TSharedRef<SWidget> UDetectiveOverlayWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Scanlines = WidgetTree->ConstructWidget<UImage>();
		if (UMaterialInterface* Source = GetDefault<UGothamUISettings>()->DetectiveOverlayMaterial.LoadSynchronous())
		{
			Material = UMaterialInstanceDynamic::Create(Source, this);
			Scanlines->SetBrushFromMaterial(Material);
		}
		else
		{
			Scanlines->SetColorAndOpacity(FLinearColor::Transparent);
		}
		UOverlaySlot* ImageSlot = Root->AddChildToOverlay(Scanlines);
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);

		// "DETECTIVE MODE   [E] Scan" strip, top centre.
		UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>();
		Prompt = Strip;
		UOverlaySlot* StripSlot = Root->AddChildToOverlay(Strip);
		StripSlot->SetHorizontalAlignment(HAlign_Center);
		StripSlot->SetVerticalAlignment(VAlign_Top);
		StripSlot->SetPadding(FMargin(0.f, 48.f, 0.f, 0.f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 22;
		Label->SetFont(Font);
		Label->SetText(LOCTEXT("Mode", "DETECTIVE MODE"));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.f)));
		Strip->AddChildToHorizontalBox(Label)->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));

		UGothamInputGlyph* ScanGlyph = WidgetTree->ConstructWidget<UGothamInputGlyph>();
		ScanGlyph->SetAction(TEXT("Scan"));
		Strip->AddChildToHorizontalBox(ScanGlyph)->SetVerticalAlignment(VAlign_Center);

		UTextBlock* ScanLabel = WidgetTree->ConstructWidget<UTextBlock>();
		ScanLabel->SetText(LOCTEXT("Scan", "Scan"));
		Strip->AddChildToHorizontalBox(ScanLabel)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UDetectiveOverlayWidget::SetViewModel(UDetectiveViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UDetectiveViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UDetectiveOverlayWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::Alpha, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsVisible, Delegate);
	}
	Refresh();
}

void UDetectiveOverlayWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UDetectiveOverlayWidget::Refresh()
{
	if (!ViewModel || !Scanlines)
	{
		return;
	}
	// Collapsed when off: no material draw, no layout cost while the mode is not in use.
	SetVisibility(ViewModel->GetIsVisible() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Material)
	{
		Material->SetScalarParameterValue(TEXT("Progress"), ViewModel->GetAlpha());
	}
	Prompt->SetRenderOpacity(ViewModel->GetAlpha());
}

#undef LOCTEXT_NAMESPACE
