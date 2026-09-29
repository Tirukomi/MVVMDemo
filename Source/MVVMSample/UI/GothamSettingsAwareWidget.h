// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Blueprint/UserWidget.h"
#include "GothamSettingsAwareWidget.generated.h"

class UGothamSettingsSubsystem;

/**
 * Base for widgets that restyle themselves from accessibility settings (palette, contrast, motion, language).
 * Subscribes while constructed and calls OnSettingsApplied() on every change.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UGothamSettingsAwareWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Re-apply anything that depends on settings. Default does nothing. */
	virtual void OnSettingsApplied() {}

	/** Current settings, or defaults if the subsystem is unavailable (e.g. in a test). */
	const FGothamSettingsData& GetGothamSettings() const;
	FLinearColor GetToken(EGothamColorToken Token) const;
	float GetPanelAlpha() const;

private:
	void HandleSettingsChanged(const FGothamSettingsData& Data) { OnSettingsApplied(); }

	FDelegateHandle SettingsHandle;
};
