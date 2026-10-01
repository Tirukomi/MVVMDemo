// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Components/Border.h"
#include "MvsSwatch.generated.h"

/**
 * A flat block of one palette colour (an accent bar, a rule) that keeps itself in the current palette: set the token
 * once and it follows colour mode and high contrast, so its screen has nothing to recolour.
 */
UCLASS()
class MVVMSAMPLE_API UMvsSwatch : public UBorder
{
	GENERATED_BODY()

public:
	void SetColorToken(EMvsColorToken InToken, float InAlpha = 1.f);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyTheme();

	EMvsColorToken Token = EMvsColorToken::Accent;
	float Alpha = 1.f;
	FMvsSettingsListener SettingsListener;
};
