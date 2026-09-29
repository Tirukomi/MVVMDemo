// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/ComboWidget.h"
#include "UI/DetectiveOverlayWidget.h"
#include "UI/ObjectiveTrackerWidget.h"
#include "UI/GadgetSlotWidget.h"
#include "UI/HealthBarWidget.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"

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
	if (!GadgetSlotClass)
	{
		GadgetSlotClass = UGadgetSlotWidget::StaticClass();
	}

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
		WidgetTree->RootWidget = Canvas;

		// Detective overlay first so it sits behind every other HUD element.
		DetectiveOverlay = WidgetTree->ConstructWidget<UDetectiveOverlayWidget>();
		UCanvasPanelSlot* OverlaySlot = Canvas->AddChildToCanvas(DetectiveOverlay);
		OverlaySlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		OverlaySlot->SetOffsets(FMargin(0.f));

		ObjectiveTracker = WidgetTree->ConstructWidget<UObjectiveTrackerWidget>();
		UCanvasPanelSlot* TrackerSlot = Canvas->AddChildToCanvas(ObjectiveTracker);
		TrackerSlot->SetAnchors(FAnchors(0.f, 0.f));
		TrackerSlot->SetPosition(FVector2D(48.f, 48.f));
		TrackerSlot->SetAutoSize(true);

		HealthBar = WidgetTree->ConstructWidget<UHealthBarWidget>();
		UCanvasPanelSlot* HealthSlot = Canvas->AddChildToCanvas(HealthBar);
		HealthSlot->SetAnchors(FAnchors(0.f, 1.f));
		HealthSlot->SetAlignment(FVector2D(0.f, 1.f));
		HealthSlot->SetPosition(FVector2D(48.f, -48.f));
		HealthSlot->SetAutoSize(true);

		GadgetBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		UCanvasPanelSlot* GadgetSlot = Canvas->AddChildToCanvas(GadgetBox);
		GadgetSlot->SetAnchors(FAnchors(0.5f, 1.f));
		GadgetSlot->SetAlignment(FVector2D(0.5f, 1.f));
		GadgetSlot->SetPosition(FVector2D(0.f, -48.f));
		GadgetSlot->SetAutoSize(true);

		ComboCounter = WidgetTree->ConstructWidget<UComboWidget>();
		UCanvasPanelSlot* ComboSlot = Canvas->AddChildToCanvas(ComboCounter);
		ComboSlot->SetAnchors(FAnchors(1.f, 0.f));
		ComboSlot->SetAlignment(FVector2D(1.f, 0.f));
		ComboSlot->SetPosition(FVector2D(-48.f, 48.f));
		ComboSlot->SetAutoSize(true);
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
	DetectiveOverlay->SetViewModel(ViewModels->GetDetective());
	ObjectiveTracker->SetViewModel(ViewModels->GetObjectives());

	// Slots are created once and re-created only if the loadout size changes.
	GadgetBarVM = ViewModels->GetGadgetBar();
	using FVM = UGadgetBarViewModel::FFieldNotificationClassDescriptor;
	GadgetBarVM->AddFieldValueChangedDelegate(FVM::Slots,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGothamHudWidget::OnGadgetBarChanged));
	RebuildGadgetSlots();
}

void UGothamHudWidget::NativeDestruct()
{
	if (GadgetBarVM)
	{
		GadgetBarVM->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void UGothamHudWidget::RebuildGadgetSlots()
{
	GadgetBox->ClearChildren();
	if (!GadgetBarVM)
	{
		return;
	}
	for (UGadgetSlotViewModel* SlotVM : GadgetBarVM->GetSlots())
	{
		UGadgetSlotWidget* Entry = CreateWidget<UGadgetSlotWidget>(this, GadgetSlotClass);
		Entry->SetViewModel(SlotVM);
		GadgetBox->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(6.f, 0.f));
	}
}
