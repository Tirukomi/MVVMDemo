// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GothamScrim.generated.h"

class SGothamScrim;

/**
 * A full-screen darkening gradient behind menus: heavy on the left where the menu column is, lighter on the right so
 * the (blurred) world still reads. Four coloured vertices, no texture.
 */
UCLASS()
class MVVMSAMPLE_API UGothamScrim : public UWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scrim")
	FLinearColor Color = FLinearColor(0.004f, 0.006f, 0.01f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scrim", meta = (ClampMin = "0", ClampMax = "1"))
	float LeftAlpha = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scrim", meta = (ClampMin = "0", ClampMax = "1"))
	float RightAlpha = 0.5f;

	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGothamScrim> MyScrim;
};
