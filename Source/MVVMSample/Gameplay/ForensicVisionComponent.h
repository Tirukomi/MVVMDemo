// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Components/ActorComponent.h"
#include "Gameplay/ForensicTypes.h"
#include "ForensicVisionComponent.generated.h"

class UCameraComponent;
class UForensicComponent;
class UMaterialInstanceDynamic;

/**
 * Presentation for Forensic Mode on the world side: drives the post-process material and a small FOV pinch
 * from the same transition alpha the UI uses, so HUD, materials and camera always move together.
 */
UCLASS(ClassGroup = (Mvs), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UForensicVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UForensicVisionComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleForensicChanged(bool bActive, float Alpha);
	void ApplySettings(const struct FMvsSettingsData& Data);
	void HandlePulse(const FVector& Center, float MaxRadius);
	void PushPulseParameters();

	/** FOV the camera eases toward at full forensic alpha. */
	UPROPERTY(EditDefaultsOnly, Category = "Forensic", meta = (ClampMin = "30", ClampMax = "120"))
	float ForensicFOV = 78.f;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VisionMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UForensicComponent> Forensic;

	float BaseFOV = 90.f;
	bool bReducedMotion = false;
	FMvsSettingsListener SettingsListener;
	FDelegateHandle PulseHandle;

	/** Ring currently travelling through the world. */
	FMvsScanPulse Pulse;
	/** Radius inside which the forensic look is shown; follows the opening pulse, then covers everything. */
	float RevealRadius = 1.0e7f;
	FVector RevealCenter = FVector::ZeroVector;
	bool bRevealing = false;
	FDelegateHandle Handle;
};
