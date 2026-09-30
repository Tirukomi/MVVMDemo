// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ThreatIndicatorLayer.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/Slate/SThreatIndicatorLayer.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/ThreatViewModel.h"

TSharedRef<SGothamWorldOverlayBase> UThreatIndicatorLayer::MakeOverlay()
{
	TSharedRef<SThreatIndicatorLayer> Layer = SNew(SThreatIndicatorLayer);
	Layer->SetProvider(MakeProvider(&UThreatIndicatorLayer::BuildIndicators));
	return Layer;
}

void UThreatIndicatorLayer::SetViewModel(UThreatViewModel* InViewModel)
{
	GothamMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	GothamMVVM::Bind(ViewModel, this, &UThreatIndicatorLayer::OnThreatsChanged,
		{ UThreatViewModel::FFieldNotificationClassDescriptor::ThreatCount });
	UpdateActive();
}

void UThreatIndicatorLayer::SetColors(const FLinearColor& InDanger, const FLinearColor& InIdle, const FLinearColor& InPanel, const FLinearColor& InText)
{
	if (SThreatIndicatorLayer* Layer = GetOverlay<SThreatIndicatorLayer>())
	{
		Layer->SetColors(InDanger, InIdle, InPanel, InText);
	}
}

void UThreatIndicatorLayer::SetReducedMotion(bool bInReduced)
{
	if (SThreatIndicatorLayer* Layer = GetOverlay<SThreatIndicatorLayer>())
	{
		Layer->SetReducedMotion(bInReduced);
	}
}

bool UThreatIndicatorLayer::ShouldBeActive() const
{
	return ViewModel && ViewModel->GetThreatCount() > 0;
}

void UThreatIndicatorLayer::OnThreatsChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	UpdateActive();
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
		Indicator.bProjected = Indicator.ViewDirection.X > 0.f && ProjectToLayer(Threat.PromptLocation, Indicator.Screen);
		bAnyWarning |= bWarning;
	}
	SThreatIndicatorLayer* Layer = GetOverlay<SThreatIndicatorLayer>();
	if (bAnyWarning && Layer)
	{
		// Follows rebinding and device switches: the prompt always names the key that counters right now.
		const FKey Key = UGothamInputGlyph::FindKeyForAction(PC, TEXT("Counter"));
		Layer->SetKeyLabel(Key.IsValid() ? UGothamInputGlyph::GetKeyLabel(Key, PC->GetLocalPlayer()) : FText::GetEmpty());
	}
}
