// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/DetectiveVisionComponent.h"

#include "Camera/CameraComponent.h"
#include "Gameplay/DetectiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UI/GothamUISettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamVision, Log, All);

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
	HandleDetectiveChanged(Detective->IsActive(), Detective->GetAlpha());
}

void UDetectiveVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Detective)
	{
		Detective->OnDetectiveChanged.Remove(Handle);
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
	Camera->SetFieldOfView(FMath::Lerp(BaseFOV, DetectiveFOV, Eased));
}
