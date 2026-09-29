// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ComboMeter.generated.h"

class SComboMeter;

/** UMG wrapper over SComboMeter. Bind Percent to a view model's decay alpha. */
UCLASS()
class MVVMSAMPLE_API UComboMeter : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter", meta = (ClampMin = "0", ClampMax = "1"))
	float Percent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter", meta = (ClampMin = "1", ClampMax = "40"))
	int32 SegmentCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter")
	FVector2D MeterSize = FVector2D(180.f, 12.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter")
	FLinearColor FilledColor = FLinearColor(0.95f, 0.75f, 0.2f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meter")
	FLinearColor EmptyColor = FLinearColor(1.f, 1.f, 1.f, 0.12f);

	UFUNCTION(BlueprintCallable, Category = "Meter")
	void SetPercent(float InPercent);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SComboMeter> SlateMeter;
};
