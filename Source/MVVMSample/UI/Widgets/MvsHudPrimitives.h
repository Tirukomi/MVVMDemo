// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "MvsHudPrimitives.generated.h"

class SDamageVignette;
class SGadgetIcon;

/** UMG wrapper over SGadgetIcon (line-art gadget icon inside a cooldown ring). */
UCLASS()
class MVVMSAMPLE_API UGadgetIcon : public UWidget
{
	GENERATED_BODY()

public:
	/** 0 glaive, 1 grapple, 2 smoke. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon", meta = (ClampMin = "0", ClampMax = "2"))
	int32 IconIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon", meta = (ClampMin = "8"))
	float IconSize = 64.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon")
	FLinearColor RingColor = FLinearColor(1.f, 1.f, 1.f, 0.2f);

	void SetIconIndex(int32 InIndex);
	void SetColors(const FLinearColor& InColor, const FLinearColor& InRing);
	void SetCooldown(float InPercent);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGadgetIcon> SlateIcon;
	float Cooldown = 0.f;
};

/** UMG wrapper over SDamageVignette. */
UCLASS()
class MVVMSAMPLE_API UDamageVignette : public UWidget
{
	GENERATED_BODY()

public:
	void SetIntensity(float InIntensity);
	void SetColor(const FLinearColor& InColor);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SDamageVignette> SlateVignette;
	float Intensity = 0.f;
	FLinearColor Color = FLinearColor(0.9f, 0.1f, 0.1f);
};
