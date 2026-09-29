// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamHudWidget.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/ClueMarkerLayer.h"
#include "UI/ComboWidget.h"
#include "UI/DetectiveOverlayWidget.h"
#include "UI/GadgetSelectorWidget.h"
#include "UI/HealthBarWidget.h"
#include "UI/ObjectiveTrackerWidget.h"
#include "UI/SubtitleWidget.h"
#include "UI/Widgets/GothamHudPrimitives.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/GothamViewModelSubsystem.h"

namespace
{
	constexpr float Margin = 44.f;
	constexpr float FlashSeconds = 0.5f;
	constexpr float LowHealthVignette = 0.35f;

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FAnchors& Anchors, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(Anchors);
		Slot->SetAlignment(Alignment);
		Slot->SetPosition(Position);
		Slot->SetAutoSize(true);
		return Slot;
	}

	void Fill(UCanvasPanel* Canvas, UWidget* Widget)
	{
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Widget);
		Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		Slot->SetOffsets(FMargin(0.f));
	}
}

UGothamHudWidget::UGothamHudWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanDismissWithBack = false;
	SetIsFocusable(false);
}

TOptional<FUIInputConfig> UGothamHudWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, false);
}

TSharedRef<SWidget> UGothamHudWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		// Full-screen layers first so they sit behind every HUD element.
		DetectiveOverlay = WidgetTree->ConstructWidget<UDetectiveOverlayWidget>();
		Fill(Canvas, DetectiveOverlay);
		ClueMarkers = WidgetTree->ConstructWidget<UClueMarkerLayer>();
		ClueMarkers->SetVisibility(ESlateVisibility::HitTestInvisible);
		Fill(Canvas, ClueMarkers);
		Vignette = WidgetTree->ConstructWidget<UDamageVignette>();
		Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
		Fill(Canvas, Vignette);

		// Top-left: health, then the combo counter under it.
		UVerticalBox* Vitals = WidgetTree->ConstructWidget<UVerticalBox>();
		Place(Canvas, Vitals, FAnchors(0.f, 0.f), FVector2D(0.f, 0.f), FVector2D(Margin, Margin));
		HealthBar = WidgetTree->ConstructWidget<UHealthBarWidget>();
		Vitals->AddChildToVerticalBox(HealthBar);
		ComboCounter = WidgetTree->ConstructWidget<UComboWidget>();
		Vitals->AddChildToVerticalBox(ComboCounter)->SetPadding(FMargin(0.f, 14.f, 0.f, 0.f));

		// Top-right: gadget selector, objective under it.
		UVerticalBox* Right = WidgetTree->ConstructWidget<UVerticalBox>();
		Place(Canvas, Right, FAnchors(1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(-Margin, Margin - 8.f));
		GadgetSelector = WidgetTree->ConstructWidget<UGadgetSelectorWidget>();
		Right->AddChildToVerticalBox(GadgetSelector)->SetHorizontalAlignment(HAlign_Right);
		ObjectiveTracker = WidgetTree->ConstructWidget<UObjectiveTrackerWidget>();
		UVerticalBoxSlot* ObjectiveSlot = Right->AddChildToVerticalBox(ObjectiveTracker);
		ObjectiveSlot->SetHorizontalAlignment(HAlign_Right);
		ObjectiveSlot->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));

		Subtitles = WidgetTree->ConstructWidget<USubtitleWidget>();
		Place(Canvas, Subtitles, FAnchors(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -90.f));
	}
	return Super::RebuildWidget();
}

void UGothamHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UGothamViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr;
	if (!ViewModels)
	{
		return;
	}

	HealthBar->SetViewModel(ViewModels->GetVitals());
	ComboCounter->SetViewModel(ViewModels->GetCombo());
	GadgetSelector->SetViewModel(ViewModels->GetGadgetBar());
	DetectiveOverlay->SetViewModel(ViewModels->GetDetective());
	ObjectiveTracker->SetViewModel(ViewModels->GetObjectives());
	Subtitles->SetViewModel(ViewModels->GetSubtitles());
	ClueMarkers->SetViewModels(ViewModels->GetClues(), ViewModels->GetDetective());
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		SettingsHandle = Settings->OnSettingsChanged.AddWeakLambda(this, [this](const FGothamSettingsData&) { ApplyMarkerColors(); UpdateVignetteRest(); });
	}
	ApplyMarkerColors();

	VitalsVM = ViewModels->GetVitals();
	LastDamageCount = VitalsVM->GetDamageCount();
	using FVM = UPlayerVitalsViewModel::FFieldNotificationClassDescriptor;
	const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGothamHudWidget::OnVitalsChanged);
	VitalsVM->AddFieldValueChangedDelegate(FVM::DamageCount, Delegate);
	VitalsVM->AddFieldValueChangedDelegate(FVM::bIsLowHealth, Delegate);
	UpdateVignetteRest();
}

void UGothamHudWidget::NativeDestruct()
{
	FTSTicker::GetCoreTicker().RemoveTicker(FlashHandle);
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		Settings->OnSettingsChanged.Remove(SettingsHandle);
	}
	if (VitalsVM)
	{
		VitalsVM->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void UGothamHudWidget::OnVitalsChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	if (VitalsVM->GetDamageCount() != LastDamageCount)
	{
		LastDamageCount = VitalsVM->GetDamageCount();
		FlashVignette();
	}
	else if (!FlashHandle.IsValid())
	{
		UpdateVignetteRest();
	}
}

void UGothamHudWidget::UpdateVignetteRest()
{
	if (Vignette && VitalsVM)
	{
		if (const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
		{
			Vignette->SetColor(Settings->GetColor(EGothamColorToken::Danger));
		}
		Vignette->SetIntensity(VitalsVM->GetIsLowHealth() ? LowHealthVignette : 0.f);
	}
}

void UGothamHudWidget::ApplyMarkerColors()
{
	if (const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		ClueMarkers->SetColors(Settings->GetColor(EGothamColorToken::Unscanned), Settings->GetColor(EGothamColorToken::Scanned),
			Settings->GetColor(EGothamColorToken::Accent), Settings->GetColor(EGothamColorToken::TextMuted));
	}
}

void UGothamHudWidget::FlashVignette()
{
	// A colour flash, not movement, so it stays on under reduced motion (it is the main "you were hit" cue).
	FTSTicker::GetCoreTicker().RemoveTicker(FlashHandle);
	FlashElapsed = 0.f;
	UpdateVignetteRest();
	Vignette->SetIntensity(1.f);
	FlashHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float Dt)
	{
		FlashElapsed += Dt;
		const float T = FMath::Clamp(FlashElapsed / FlashSeconds, 0.f, 1.f);
		const float Rest = VitalsVM && VitalsVM->GetIsLowHealth() ? LowHealthVignette : 0.f;
		Vignette->SetIntensity(FMath::Lerp(1.f, Rest, FMath::InterpEaseOut(0.f, 1.f, T, 2.f)));
		if (T >= 1.f)
		{
			FlashHandle.Reset();
			return false;
		}
		return true;
	}));
}
