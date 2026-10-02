// Copyright IG. All Rights Reserved.

#include "UI/ThreatIndicatorLayer.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/Slate/SThreatIndicatorLayer.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/ThreatViewModel.h"

TSharedRef<SMvsWorldOverlayBase> UThreatIndicatorLayer::MakeOverlay()
{
	TSharedRef<SThreatIndicatorLayer> Layer = SNew(SThreatIndicatorLayer);
	Layer->SetProvider(MakeProvider(&UThreatIndicatorLayer::BuildIndicators));
	return Layer;
}

void UThreatIndicatorLayer::SetViewModel(UThreatViewModel* InViewModel)
{
	MvsMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	MvsMVVM::Bind(ViewModel, this, &UThreatIndicatorLayer::OnThreatsChanged,
		{ UThreatViewModel::FFieldNotificationClassDescriptor::ThreatCount });
	UpdateActive();
}

void UThreatIndicatorLayer::ApplyTheme(const FMvsTheme& Theme)
{
	if (SThreatIndicatorLayer* Layer = GetOverlay<SThreatIndicatorLayer>())
	{
		Layer->SetColors(Theme.Color(EMvsColorToken::Danger), Theme.Color(EMvsColorToken::TextMuted),
			Theme.Color(EMvsColorToken::Panel), Theme.Color(EMvsColorToken::TextPrimary));
		Layer->SetReducedMotion(Theme.bReducedMotion);
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

void UThreatIndicatorLayer::BuildIndicators(TArray<FMvsThreatIndicator>& Out) const
{
	APlayerController* PC = GetOwningPlayer();
	if (!ViewModel || !PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FRotator CameraRotation = PC->PlayerCameraManager->GetCameraRotation();
	bool bAnyWarning = false;
	for (const FMvsThreatSnapshot& Threat : ViewModel->GetThreats())
	{
		const bool bWarning = Threat.State == EMvsThugState::Warning;
		if (!bWarning && Threat.Distance > ArrowRange)
		{
			continue;
		}
		FMvsThreatIndicator& Indicator = Out.AddDefaulted_GetRef();
		Indicator.bWarning = bWarning;
		Indicator.Progress = Threat.WarningProgress;
		// Stunned thugs are no threat for now: their arrow dims.
		Indicator.Opacity = Threat.State == EMvsThugState::Stunned ? 0.35f : 1.f;
		Indicator.ViewDirection = CameraRotation.UnrotateVector(Threat.PromptLocation - CameraLocation);
		Indicator.bProjected = Indicator.ViewDirection.X > 0.f && ProjectToLayer(Threat.PromptLocation, Indicator.Screen);
		bAnyWarning |= bWarning;
	}
	SThreatIndicatorLayer* Layer = GetOverlay<SThreatIndicatorLayer>();
	if (bAnyWarning && Layer)
	{
		// Follows rebinding and device switches: the prompt always names the key that counters right now.
		const FKey Key = UMvsInputGlyph::FindKeyForAction(PC, TEXT("Counter"));
		Layer->SetKeyLabel(Key.IsValid() ? UMvsInputGlyph::GetKeyLabel(Key, PC->GetLocalPlayer()) : FText::GetEmpty());
	}
}
