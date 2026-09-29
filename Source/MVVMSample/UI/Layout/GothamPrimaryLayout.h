// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "UI/Layout/GothamUITypes.h"
#include "GothamPrimaryLayout.generated.h"

class UCommonActivatableWidgetStack;

/** Root of all UI for a local player: one activatable-widget stack per EGothamUILayer, stacked in z-order. */
UCLASS()
class MVVMSAMPLE_API UGothamPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayer(EGothamUILayer Layer) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonActivatableWidgetStack>> Layers;
};
