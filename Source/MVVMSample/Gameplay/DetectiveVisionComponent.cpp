// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/DetectiveVisionComponent.h"

#include "Camera/CameraComponent.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Gameplay/DetectiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/GothamUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamVision, Log, All);

UDetectiveVisionComponent::UDetectiveVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDetectiveVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	Detective = GetOwner()->FindComponentByClass<UDetectiveComponent>();
	if (!Camera || !Detective)
	{
		return;
	}
	BaseFOV = Camera->FieldOfView;

	if (UMaterialInterface* Source = GetDefault<UGothamUISettings>()->DetectiveVisionMaterial.LoadSynchronous())
	{
		VisionMaterial = UMaterialInstanceDynamic::Create(Source, this);
		Camera->PostProcessSettings.AddBlendable(VisionMaterial, 0.f);
	}
	else
	{
		UE_LOG(LogGothamVision, Warning, TEXT("Detective vision material is not set or missing; only the camera FOV pinch will play."));
	}

	Handle = Detective->OnDetectiveChanged.AddUObject(this, &UDetectiveVisionComponent::HandleDetectiveChanged);
	PulseHandle = Detective->OnScanPulse.AddUObject(this, &UDetectiveVisionComponent::HandlePulse);
	PushPulseParameters();
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		SettingsHandle = Settings->OnSettingsChanged.AddUObject(this, &UDetectiveVisionComponent::ApplySettings);
		ApplySettings(Settings->GetSettings());
	}
	HandleDetectiveChanged(Detective->IsActive(), Detective->GetAlpha());
}

void UDetectiveVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Detective)
	{
		Detective->OnDetectiveChanged.Remove(Handle);
		Detective->OnScanPulse.Remove(PulseHandle);
	}
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		Settings->OnSettingsChanged.Remove(SettingsHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UDetectiveVisionComponent::HandleDetectiveChanged(bool bActive, float Alpha)
{
	// Ease so the pinch and the material wipe feel like one motion rather than linear ramps.
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	if (VisionMaterial)
	{
		VisionMaterial->SetScalarParameterValue(TEXT("Alpha"), Eased);
		// Weight only gates whether the pass runs at all; the material blends itself with Alpha.
		Camera->PostProcessSettings.WeightedBlendables.Array[0].Weight = Alpha > 0.f ? 1.f : 0.f;
	}
	Camera->SetFieldOfView(FMath::Lerp(BaseFOV, bReducedMotion ? BaseFOV : DetectiveFOV, Eased));
}

/** Clue colours come from the accessibility palette, so colour-blind presets recolour the world pass too. */
void UDetectiveVisionComponent::ApplySettings(const FGothamSettingsData& Data)
{
	bReducedMotion = Data.bReducedMotion;
	if (Detective)
	{
		Detective->SetReducedMotion(Data.bReducedMotion);
	}
	if (VisionMaterial)
	{
		VisionMaterial->SetVectorParameterValue(TEXT("UnscannedColor"), GothamPalette::Resolve(EGothamColorToken::Unscanned, Data.ColorMode, Data.bHighContrast));
		VisionMaterial->SetVectorParameterValue(TEXT("ScannedColor"), GothamPalette::Resolve(EGothamColorToken::Scanned, Data.ColorMode, Data.bHighContrast));
		VisionMaterial->SetVectorParameterValue(TEXT("HostileColor"), GothamPalette::Resolve(EGothamColorToken::Danger, Data.ColorMode, Data.bHighContrast));
		// Interactables are context, not targets: a faint fill and halo so they never out-shout clues.
		VisionMaterial->SetVectorParameterValue(TEXT("InteractColor"), GothamPalette::Resolve(EGothamColorToken::TextPrimary, Data.ColorMode, Data.bHighContrast) * 0.22f);
	}
	if (Camera && Detective)
	{
		HandleDetectiveChanged(Detective->IsActive(), Detective->GetAlpha());
	}
}

void UDetectiveVisionComponent::HandlePulse(const FVector& Center, float MaxRadius)
{
	if (bReducedMotion)
	{
		// No travelling ring and no radial reveal: the look simply crossfades in.
		bRevealing = false;
		RevealRadius = 1.0e7f;
		PushPulseParameters();
		return;
	}
	Pulse.Fire(Center, MaxRadius);
	// The opening pulse also reveals the detective view outward from the player; other pulses leave it fully on.
	bRevealing = FMath::IsNearlyEqual(MaxRadius, UDetectiveComponent::OpenPulseRadius);
	RevealCenter = Center;
	RevealRadius = bRevealing ? 0.f : 1.0e7f;
	PushPulseParameters();
	SetComponentTickEnabled(true);
}

void UDetectiveVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const bool bStillGoing = Pulse.Advance(DeltaTime);
	if (bRevealing)
	{
		// Lead the ring slightly so the reveal edge sits just inside the glow.
		RevealRadius = bStillGoing ? Pulse.Radius - 150.f : 1.0e7f;
		bRevealing = bStillGoing;
	}
	PushPulseParameters();
	if (!bStillGoing)
	{
		SetComponentTickEnabled(false);
	}
}

void UDetectiveVisionComponent::PushPulseParameters()
{
	if (!VisionMaterial)
	{
		return;
	}
	VisionMaterial->SetVectorParameterValue(TEXT("PulseCenter"), FLinearColor(Pulse.Center.X, Pulse.Center.Y, Pulse.Center.Z, 0.f));
	VisionMaterial->SetScalarParameterValue(TEXT("PulseRadius"), Pulse.Radius);
	VisionMaterial->SetScalarParameterValue(TEXT("PulseStrength"), Pulse.bActive ? Pulse.Strength : 0.f);
	VisionMaterial->SetVectorParameterValue(TEXT("RevealCenter"), FLinearColor(RevealCenter.X, RevealCenter.Y, RevealCenter.Z, 0.f));
	VisionMaterial->SetScalarParameterValue(TEXT("RevealRadius"), FMath::Max(0.f, RevealRadius));
}
