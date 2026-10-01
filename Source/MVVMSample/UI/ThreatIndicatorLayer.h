// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MvsWorldOverlayLayer.h"
#include "ThreatIndicatorLayer.generated.h"

class UThreatViewModel;

/**
 * Counter prompts and off-screen threat arrows. Reads the threat view model, projects each hostile with the owning
 * player's camera and hands the result to SThreatIndicatorLayer. Its active timer runs only while hostiles exist.
 */
UCLASS()
class MVVMSAMPLE_API UThreatIndicatorLayer : public UMvsWorldOverlayLayer
{
	GENERATED_BODY()

public:
	/** Idle hostiles further than this get no arrow (warning ones always do). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Threats", meta = (ClampMin = "100"))
	float ArrowRange = 2500.f;

	void SetViewModel(UThreatViewModel* InViewModel);

protected:
	virtual TSharedRef<SMvsWorldOverlayBase> MakeOverlay() override;
	virtual bool ShouldBeActive() const override;
	virtual void ApplyTheme() override;

private:
	void OnThreatsChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void BuildIndicators(TArray<struct FMvsThreatIndicator>& Out) const;

	UPROPERTY(Transient)
	TObjectPtr<UThreatViewModel> ViewModel;
};
