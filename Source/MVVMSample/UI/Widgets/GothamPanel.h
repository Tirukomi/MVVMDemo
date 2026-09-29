// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ContentWidget.h"
#include "GothamPanel.generated.h"

class SGothamPanel;

/** UMG wrapper over SGothamPanel: a chamfered panel with one child. Designers set shape and colours as properties. */
UCLASS()
class MVVMSAMPLE_API UGothamPanel : public UContentWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FMargin Padding = FMargin(12.f, 8.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (ClampMin = "0"))
	float Corner = 10.f;

	/** Bit mask: 1 top-left, 2 top-right, 4 bottom-right, 8 bottom-left. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (Bitmask))
	uint8 ChamferMask = 2 | 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FLinearColor FillColor = FLinearColor(0.012f, 0.015f, 0.02f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FLinearColor EdgeColor = FLinearColor(0.32f, 0.38f, 0.46f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (ClampMin = "0"))
	float EdgeThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FLinearColor AccentColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (ClampMin = "0"))
	float AccentWidth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel")
	FLinearColor GlowColor = FLinearColor::Transparent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Panel", meta = (ClampMin = "0"))
	float GlowSize = 0.f;

	void SetShape(float InCorner, uint8 InMask);
	void SetColors(const FLinearColor& InFill, const FLinearColor& InEdge, float InEdgeThickness = 1.f);
	void SetAccent(const FLinearColor& InColor, float InWidth);
	void SetGlow(const FLinearColor& InColor, float InSize);
	void SetPanelPadding(const FMargin& InPadding);

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;

private:
	TSharedPtr<SGothamPanel> MyPanel;
};
