// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/DetectiveOverlayWidget.h"
#include "UI/Style/GothamStyle.h"

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
#include "ViewModels/GothamMVVM.h"

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
		Label->SetFont(GothamStyle::Font(EGothamTextStyle::Header));
		Label->SetTextTransformPolicy(ETextTransformPolicy::ToUpper);
		Label->SetText(LOCTEXT("Mode", "DETECTIVE MODE"));
		ModeLabel = Label;
		UHorizontalBoxSlot* ModeSlot = Strip->AddChildToHorizontalBox(Label);
		ModeSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
		ModeSlot->SetVerticalAlignment(VAlign_Center);

		UGothamInputGlyph* ScanGlyph = WidgetTree->ConstructWidget<UGothamInputGlyph>();
		ScanGlyph->SetAction(TEXT("Scan"));
		Strip->AddChildToHorizontalBox(ScanGlyph)->SetVerticalAlignment(VAlign_Center);

		ScanLabel = WidgetTree->ConstructWidget<UTextBlock>();
		// Centred on the key glyph: the default Fill alignment would pin the text to the top of the row.
		UHorizontalBoxSlot* ScanSlot = Strip->AddChildToHorizontalBox(ScanLabel);
		ScanSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		ScanSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Super::RebuildWidget();
}

void UDetectiveOverlayWidget::SetViewModel(UDetectiveViewModel* InViewModel)
{
	using FVM = UDetectiveViewModel::FFieldNotificationClassDescriptor;
	GothamMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	GothamMVVM::Bind(ViewModel, this, &UDetectiveOverlayWidget::OnFieldChanged, { FVM::Alpha, FVM::bIsVisible });
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
	ModeLabel->SetColorAndOpacity(FSlateColor(GetToken(EGothamColorToken::Info)));
	// The prompt names the input the player actually has to perform.
	GothamStyle::ApplyText(ScanLabel, EGothamTextStyle::Label, GetToken(EGothamColorToken::TextPrimary));
	ScanLabel->SetText(GetGothamSettings().ScanMode == EGothamScanMode::Tap ? LOCTEXT("Scan", "Scan") : LOCTEXT("HoldToAnalyse", "Hold to analyse"));
	if (Material)
	{
		// The scanline scroll and wipe sweep freeze in reduced-motion mode; the tint and vignette stay.
		Material->SetScalarParameterValue(TEXT("MotionScale"), GetGothamSettings().bReducedMotion ? 0.f : 1.f);
		const FLinearColor Tint = GetToken(EGothamColorToken::Info);
		Material->SetVectorParameterValue(TEXT("Tint"), Tint);
	}
}

#undef LOCTEXT_NAMESPACE
