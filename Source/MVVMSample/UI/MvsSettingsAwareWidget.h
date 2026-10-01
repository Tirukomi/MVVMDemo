// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Blueprint/UserWidget.h"
#include "MvsSettingsAwareWidget.generated.h"

class UMvsSettingsSubsystem;

/**
 * Base for widgets that restyle themselves from accessibility settings (palette, contrast, motion, language).
 * Subscribes while constructed and calls ApplyTheme() on every change.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UMvsSettingsAwareWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Re-apply anything that depends on settings. Default does nothing. */
	virtual void ApplyTheme() {}

	/** Current settings, or defaults if the subsystem is unavailable (e.g. in a test). */
	const FMvsSettingsData& GetMvsSettings() const;
	FLinearColor GetToken(EMvsColorToken Token) const;
	float GetPanelAlpha() const;

private:
	FMvsSettingsListener SettingsListener;
};
