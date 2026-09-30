// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ThreatIndicatorLayer.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/Slate/SThreatIndicatorLayer.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/ThreatViewModel.h"

TSharedRef<SWidget> UThreatIndicatorLayer::RebuildWidget()
{
	SlateLayer = SNew(SThreatIndicatorLayer);
	const TWeakObjectPtr<UThreatIndicatorLayer> Weak(this);
	SlateLayer->SetProvider([Weak](TArray<FGothamThreatIndicator>& Out)
	{
		if (const UThreatIndicatorLayer* Self = Weak.Get())
		{
			Self->BuildIndicators(Out);
		}
	});
	SlateLayer->SetActive(ShouldBeActive());
	return SlateLayer.ToSharedRef();
}

void UThreatIndicatorLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateLayer.Reset();
}

void UThreatIndicatorLayer::SetViewModel(UThreatViewModel* InViewModel)
{
	GothamMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	GothamMVVM::Bind(ViewModel, this, &UThreatIndicatorLayer::OnThreatsChanged,
		{ UThreatViewModel::FFieldNotificationClassDescriptor::ThreatCount });
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetActive(ShouldBeActive());
	}
}

void UThreatIndicatorLayer::SetColors(const FLinearColor& InDanger, const FLinearColor& InIdle, const FLinearColor& InPanel, const FLinearColor& InText)
{
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetColors(InDanger, InIdle, InPanel, InText);
	}
}

void UThreatIndicatorLayer::SetReducedMotion(bool bInReduced)
{
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetReducedMotion(bInReduced);
	}
}

bool UThreatIndicatorLayer::ShouldBeActive() const
{
	return ViewModel && ViewModel->GetThreatCount() > 0;
}

void UThreatIndicatorLayer::OnThreatsChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetActive(ShouldBeActive());
	}
}

void UThreatIndicatorLayer::BuildIndicators(TArray<FGothamThreatIndicator>& Out) const
{
	APlayerController* PC = GetOwningPlayer();
	if (!ViewModel || !PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
	bool bAnyWarning = false;
	for (const FGothamThreatSnapshot& Threat : ViewModel->GetThreats())
	{
		const bool bWarning = Threat.State == EGothamThugState::Warning;
		if (!bWarning && Threat.Distance > ArrowRange)
		{
			continue;
		}
		FGothamThreatIndicator& Indicator = Out.AddDefaulted_GetRef();
		Indicator.bWarning = bWarning;
		Indicator.Progress = Threat.WarningProgress;
		// Stunned thugs are no threat for now: their arrow dims.
		Indicator.Opacity = Threat.State == EGothamThugState::Stunned ? 0.35f : 1.f;
		Indicator.ViewDirection = CameraRotation.UnrotateVector(Threat.PromptLocation - CameraLocation);
		// Viewport-relative and DPI-adjusted: the same space as the full-screen HUD canvas this layer fills.
		Indicator.bProjected = Indicator.ViewDirection.X > 0.f
			&& UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Threat.PromptLocation, Indicator.Screen, true);
		bAnyWarning |= bWarning;
	}
	if (bAnyWarning && SlateLayer.IsValid())
	{
		// Follows rebinding and device switches: the prompt always names the key that counters right now.
		const FKey Key = UGothamInputGlyph::FindKeyForAction(PC, TEXT("Counter"));
		SlateLayer->SetKeyLabel(Key.IsValid() ? UGothamInputGlyph::GetKeyLabel(Key) : FText::GetEmpty());
	}
}
