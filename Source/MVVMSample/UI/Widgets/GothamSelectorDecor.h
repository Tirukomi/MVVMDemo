// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GothamSelectorDecor.generated.h"

class SGothamSelectorDecor;

/**
 * The frame of a value selector: a chevron at each end and one pip per choice along the bottom, the current one lit.
 * The value text sits on top of it (a normal text block), so this draws only lines and boxes.
 */
UCLASS()
class MVVMSAMPLE_API UGothamSelectorDecor : public UWidget
{
	GENERATED_BODY()

public:
	/** Index of Count choices; bWraps false dims the chevron at either end. */
	void SetPosition(int32 InIndex, int32 InCount, bool bInWraps);
	void SetColors(const FLinearColor& InActive, const FLinearColor& InIdle, bool bInFocused);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGothamSelectorDecor> MyDecor;
	int32 Index = 0;
	int32 Count = 1;
	bool bWraps = true;
	FLinearColor Active = FLinearColor::White;
	FLinearColor Idle = FLinearColor::Gray;
	bool bFocused = false;
};
