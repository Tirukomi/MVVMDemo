// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "GadgetWheelScreen.generated.h"

class UGadgetBarViewModel;
class UGadgetWheel;

/**
 * Hold-to-open gadget wheel. Slows time while open; releasing the open key uses the hovered gadget (in toggle mode,
 * pressing it again does), Back cancels. The open key is the GadgetWheel action's, bound with Common UI, so it follows
 * rebinding. Items come from the gadget bar view model, so cooldowns update live inside the wheel.
 */
UCLASS()
class MVVMSAMPLE_API UGadgetWheelScreen : public UGothamScreen
{
	GENERATED_BODY()

public:
	UGadgetWheelScreen(const FObjectInitializer& ObjectInitializer);

	/** Feeds an analog stick (+Y up) to the wheel; used when focus is elsewhere and by dev tooling. */
	void SetStickInput(FVector2D Stick);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnClosed() override;
	virtual void ApplyTheme() override;

private:
	/** The open key went down (Toggle) or up (Hold): use the hovered gadget, or close if none is hovered. */
	void HandleOpenKey(EInputEvent Event);
	UFUNCTION()
	void HandleItemSelected(int32 ItemIndex);

	void RefreshItems();
	void OnSlotChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshItems(); }

	UPROPERTY(Transient)
	TObjectPtr<UGadgetWheel> Wheel;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBar;

	bool bOpenKeyBound = false;
};
