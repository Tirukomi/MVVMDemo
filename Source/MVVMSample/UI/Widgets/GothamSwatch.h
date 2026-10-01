// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Components/Border.h"
#include "GothamSwatch.generated.h"

/**
 * A flat block of one palette colour (an accent bar, a rule) that keeps itself in the current palette: set the token
 * once and it follows colour mode and high contrast, so its screen has nothing to recolour.
 */
UCLASS()
class MVVMSAMPLE_API UGothamSwatch : public UBorder
{
	GENERATED_BODY()

public:
	void SetColorToken(EGothamColorToken InToken, float InAlpha = 1.f);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyTheme();

	EGothamColorToken Token = EGothamColorToken::Accent;
	float Alpha = 1.f;
	FGothamSettingsListener SettingsListener;
};
