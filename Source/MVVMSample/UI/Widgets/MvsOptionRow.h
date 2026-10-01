// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "CommonButtonBase.h"
#include "UI/Widgets/MvsAcceptable.h"
#include "MvsOptionRow.generated.h"

class UMvsSelectorDecor;
class USettingsViewModel;
class UTextBlock;

/**
 * One settings row: the label on the left and a compact value selector (chevrons, value, position pips) on the right.
 * A Common UI button, so focus, click routing, input-method handling and sounds are Common UI's; the whole row is one
 * focus stop. Left / right (keys, d-pad or stick) change the value instead of moving focus, a click (Enter, Space,
 * gamepad A) steps forward, and a mouse click on the left half of the selector steps back. Hover moves focus, so the
 * menu highlight follows the mouse too.
 */
UCLASS()
class MVVMSAMPLE_API UMvsOptionRow : public UCommonButtonBase, public IMvsAcceptable
{
	GENERATED_BODY()

public:
	UMvsOptionRow(const FObjectInitializer& ObjectInitializer);

	/** The click Enter would make: steps the value forward (IMvsAcceptable). */
	virtual void Accept() override { HandleButtonClicked(); }

	void Setup(EMvsSetting InSetting, USettingsViewModel* InViewModel);
	EMvsSetting GetSetting() const { return Setting; }

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnClicked() override;
	virtual void NativeOnHovered() override;
	virtual void HandleFocusReceived() override;
	virtual void HandleFocusLost() override;

private:
	void Step(int32 Direction);
	void Refresh();
	void ApplyState();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	EMvsSetting Setting = EMvsSetting::Language;
	bool bFocused = false;
	/** Set by a mouse press, read by the click it produces: which way the clicked half of the selector steps. */
	TOptional<int32> PointerDirection;
	FMvsSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	UPROPERTY(Transient)
	TObjectPtr<UMvsSelectorDecor> Decor;
};
