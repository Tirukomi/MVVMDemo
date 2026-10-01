// Copyright IG. All Rights Reserved.

#include "Gameplay/ForensicVisionComponent.h"

#include "Camera/CameraComponent.h"
#include "Accessibility/MvsSettingsSubsystem.h"
#include "Gameplay/ForensicComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/MvsUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsVision, Log, All);

UForensicVisionComponent::UForensicVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UForensicVisionComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	Forensic = GetOwner()->FindComponentByClass<UForensicComponent>();
	if (!Camera || !Forensic)
	{
		return;
	}
	BaseFOV = Camera->FieldOfView;

	if (UMaterialInterface* Source = GetDefault<UMvsUISettings>()->ForensicVisionMaterial.LoadSynchronous())
	{
		VisionMaterial = UMaterialInstanceDynamic::Create(Source, this);
		Camera->PostProcessSettings.AddBlendable(VisionMaterial, 0.f);
	}
	else
	{
		UE_LOG(LogMvsVision, Warning, TEXT("Forensic vision material is not set or missing; only the camera FOV pinch will play."));
	}

	Handle = Forensic->OnForensicChanged.AddUObject(this, &UForensicVisionComponent::HandleForensicChanged);
	PulseHandle = Forensic->OnScanPulse.AddUObject(this, &UForensicVisionComponent::HandlePulse);
	PushPulseParameters();
	if (UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this))
	{
		SettingsListener.Bind(Settings, this, [this](const FMvsSettingsData& Data) { ApplySettings(Data); });
		ApplySettings(Settings->GetSettings());
	}
	HandleForensicChanged(Forensic->IsActive(), Forensic->GetAlpha());
}

void UForensicVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Forensic)
	{
		Forensic->OnForensicChanged.Remove(Handle);
		Forensic->OnScanPulse.Remove(PulseHandle);
	}
	SettingsListener.Reset();
	Super::EndPlay(EndPlayReason);
}

void UForensicVisionComponent::HandleForensicChanged(bool bActive, float Alpha)
{
	// Ease so the pinch and the material wipe feel like one motion rather than linear ramps.
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	if (VisionMaterial)
	{
		VisionMaterial->SetScalarParameterValue(TEXT("Alpha"), Eased);
		// Weight only gates whether the pass runs at all; the material blends itself with Alpha.
		Camera->PostProcessSettings.WeightedBlendables.Array[0].Weight = Alpha > 0.f ? 1.f : 0.f;
	}
	Camera->SetFieldOfView(FMath::Lerp(BaseFOV, bReducedMotion ? BaseFOV : ForensicFOV, Eased));
}

/** Clue colours come from the accessibility palette, so colour-blind presets recolour the world pass too. */
void UForensicVisionComponent::ApplySettings(const FMvsSettingsData& Data)
{
	bReducedMotion = Data.bReducedMotion;
	if (Forensic)
	{
		Forensic->SetReducedMotion(Data.bReducedMotion);
	}
	if (VisionMaterial)
	{
		VisionMaterial->SetVectorParameterValue(TEXT("UnscannedColor"), MvsPalette::Resolve(EMvsColorToken::Unscanned, Data.ColorMode, Data.bHighContrast));
		VisionMaterial->SetVectorParameterValue(TEXT("ScannedColor"), MvsPalette::Resolve(EMvsColorToken::Scanned, Data.ColorMode, Data.bHighContrast));
		VisionMaterial->SetVectorParameterValue(TEXT("HostileColor"), MvsPalette::Resolve(EMvsColorToken::Danger, Data.ColorMode, Data.bHighContrast));
		// Interactables are context, not targets: a faint fill and halo so they never out-shout clues.
		VisionMaterial->SetVectorParameterValue(TEXT("InteractColor"), MvsPalette::Resolve(EMvsColorToken::TextPrimary, Data.ColorMode, Data.bHighContrast) * 0.22f);
	}
	if (Camera && Forensic)
	{
		HandleForensicChanged(Forensic->IsActive(), Forensic->GetAlpha());
	}
}

void UForensicVisionComponent::HandlePulse(const FVector& Center, float MaxRadius)
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
	// The opening pulse also reveals the forensic view outward from the player; other pulses leave it fully on.
	bRevealing = FMath::IsNearlyEqual(MaxRadius, UForensicComponent::OpenPulseRadius);
	RevealCenter = Center;
	RevealRadius = bRevealing ? 0.f : 1.0e7f;
	PushPulseParameters();
	SetComponentTickEnabled(true);
}

void UForensicVisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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

void UForensicVisionComponent::PushPulseParameters()
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
