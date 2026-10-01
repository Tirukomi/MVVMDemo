// Copyright IG. All Rights Reserved.

#include "UI/ForensicOverlayWidget.h"
#include "UI/Style/MvsStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/MvsUISettings.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "UI/Widgets/MvsText.h"
#include "ViewModels/ForensicViewModel.h"
#include "ViewModels/MvsMVVM.h"

#define LOCTEXT_NAMESPACE "Mvs.ForensicOverlay"

TSharedRef<SWidget> UForensicOverlayWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Scanlines = WidgetTree->ConstructWidget<UImage>();
		if (UMaterialInterface* Source = GetDefault<UMvsUISettings>()->ForensicOverlayMaterial.LoadSynchronous())
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

		// "FORENSIC MODE   [E] Scan" strip, top centre.
		UHorizontalBox* Strip = WidgetTree->ConstructWidget<UHorizontalBox>();
		Prompt = Strip;
		UOverlaySlot* StripSlot = Root->AddChildToOverlay(Strip);
		StripSlot->SetHorizontalAlignment(HAlign_Center);
		StripSlot->SetVerticalAlignment(VAlign_Top);
		StripSlot->SetPadding(FMargin(0.f, 48.f, 0.f, 0.f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UMvsText>();
		MvsStyle::SetTextStyle(Label, EMvsTextStyle::Header);
		MvsText::SetUpperCase(Label, true);
		Label->SetText(LOCTEXT("Mode", "FORENSIC MODE"));
		ModeLabel = Label;
		UHorizontalBoxSlot* ModeSlot = Strip->AddChildToHorizontalBox(Label);
		ModeSlot->SetPadding(FMargin(0.f, 0.f, 24.f, 0.f));
		ModeSlot->SetVerticalAlignment(VAlign_Center);

		UMvsInputGlyph* ScanGlyph = WidgetTree->ConstructWidget<UMvsInputGlyph>();
		ScanGlyph->SetAction(TEXT("Scan"));
		Strip->AddChildToHorizontalBox(ScanGlyph)->SetVerticalAlignment(VAlign_Center);

		ScanLabel = WidgetTree->ConstructWidget<UMvsText>();
		// Centred on the key glyph: the default Fill alignment would pin the text to the top of the row.
		UHorizontalBoxSlot* ScanSlot = Strip->AddChildToHorizontalBox(ScanLabel);
		ScanSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		ScanSlot->SetVerticalAlignment(VAlign_Center);
	}
	return Super::RebuildWidget();
}

void UForensicOverlayWidget::SetViewModel(UForensicViewModel* InViewModel)
{
	using FVM = UForensicViewModel::FFieldNotificationClassDescriptor;
	MvsMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	MvsMVVM::Bind(ViewModel, this, &UForensicOverlayWidget::OnFieldChanged, { FVM::Alpha, FVM::bIsVisible });
	ApplyTheme();
	ApplyFade();
}

void UForensicOverlayWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UForensicOverlayWidget::OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	// Alpha changes every frame of the fade; the style only needs refreshing when the overlay appears.
	if (FieldId == UForensicViewModel::FFieldNotificationClassDescriptor::bIsVisible && ViewModel && ViewModel->GetIsVisible())
	{
		ApplyTheme();
	}
	ApplyFade();
}

void UForensicOverlayWidget::ApplyFade()
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

void UForensicOverlayWidget::ApplyTheme()
{
	if (!Scanlines)
	{
		return;
	}
	ModeLabel->SetColorAndOpacity(FSlateColor(GetToken(EMvsColorToken::Info)));
	// The prompt names the input the player actually has to perform.
	MvsStyle::ApplyText(ScanLabel, EMvsTextStyle::Label, GetToken(EMvsColorToken::TextPrimary));
	ScanLabel->SetText(GetMvsSettings().ScanMode == EMvsScanMode::Tap ? LOCTEXT("Scan", "Scan") : LOCTEXT("HoldToAnalyse", "Hold to analyse"));
	if (Material)
	{
		// The scanline scroll and wipe sweep freeze in reduced-motion mode; the tint and vignette stay.
		Material->SetScalarParameterValue(TEXT("MotionScale"), GetMvsSettings().bReducedMotion ? 0.f : 1.f);
		Material->SetVectorParameterValue(TEXT("Tint"), GetToken(EMvsColorToken::Info));
	}
}

#undef LOCTEXT_NAMESPACE
