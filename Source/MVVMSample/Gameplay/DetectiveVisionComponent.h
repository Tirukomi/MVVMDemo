// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Components/ActorComponent.h"
#include "Gameplay/DetectiveTypes.h"
#include "DetectiveVisionComponent.generated.h"

class UCameraComponent;
class UDetectiveComponent;
class UMaterialInstanceDynamic;

/**
 * Presentation for Detective Mode on the world side: drives the post-process material and a small FOV pinch
 * from the same transition alpha the UI uses, so HUD, materials and camera always move together.
 */
UCLASS(ClassGroup = (Gotham), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UDetectiveVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDetectiveVisionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleDetectiveChanged(bool bActive, float Alpha);
	void ApplySettings(const struct FGothamSettingsData& Data);
	void HandlePulse(const FVector& Center, float MaxRadius);
	void PushPulseParameters();

	/** FOV the camera eases toward at full detective alpha. */
	UPROPERTY(EditDefaultsOnly, Category = "Detective", meta = (ClampMin = "30", ClampMax = "120"))
	float DetectiveFOV = 78.f;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VisionMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveComponent> Detective;

	float BaseFOV = 90.f;
	bool bReducedMotion = false;
	FGothamSettingsListener SettingsListener;
	FDelegateHandle PulseHandle;

	/** Ring currently travelling through the world. */
	FGothamScanPulse Pulse;
	/** Radius inside which the detective look is shown; follows the opening pulse, then covers everything. */
	float RevealRadius = 1.0e7f;
	FVector RevealCenter = FVector::ZeroVector;
	bool bRevealing = false;
	FDelegateHandle Handle;
};
