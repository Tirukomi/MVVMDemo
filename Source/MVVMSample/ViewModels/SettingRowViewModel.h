// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "MVVMViewModelBase.h"
#include "SettingRowViewModel.generated.h"

class USettingsViewModel;

/**
 * One option of the settings screen (second review 14): what its row shows, each field notifying on its own, so a
 * view (an option row, a designer's widget) binds per option and a change re-reads only the row it touched. Owned and
 * updated by USettingsViewModel; Step edits the option through it.
 */
UCLASS(BlueprintType)
class MVVMSAMPLE_API USettingRowViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void Initialize(USettingsViewModel* InOwner, EMvsSetting InSetting);
	/**
	 * Re-reads the row from Data. Texts notify when what they display changed; bRetext notifies them regardless (after
	 * a language switch the same localized text reads differently, so comparing would miss it).
	 */
	void Update(const FMvsSettingsData& Data, bool bRetext);

	/** Steps the option (wrapping, except UI scale, which stops at its ends). Previews immediately. */
	void Step(int32 Direction);

	EMvsSetting GetSetting() const { return Setting; }
	const FText& GetLabel() const { return Label; }
	const FText& GetValueText() const { return ValueText; }
	const FText& GetDescription() const { return Description; }
	int32 GetChoiceIndex() const { return ChoiceIndex; }
	int32 GetChoiceCount() const { return ChoiceCount; }
	bool GetWraps() const { return bWraps; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText Label;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText ValueText;

	/** One or two sentences for the detail pane. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText Description;

	/** The value's position among its choices, for the selector's pips. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 ChoiceIndex = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 ChoiceCount = 1;

	/** Whether stepping past the last choice wraps to the first. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetWraps, meta = (AllowPrivateAccess = "true"))
	bool bWraps = true;

private:
	void SetText(FText& Field, const FText& NewValue, UE::FieldNotification::FFieldId FieldId, bool bForce);

	EMvsSetting Setting = EMvsSetting::Count;
	TWeakObjectPtr<USettingsViewModel> Owner;
};
