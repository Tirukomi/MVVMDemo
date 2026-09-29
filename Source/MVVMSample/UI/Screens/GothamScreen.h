// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GothamScreen.generated.h"

class UGothamButton;
class UGothamInputGlyph;
class UTextBlock;
class UVerticalBox;

/**
 * Base for every full screen and modal. Gives menus their input config (cursor, UI-only input),
 * Esc / gamepad B to go back, and default focus so gamepad navigation always has somewhere to start.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UGothamScreen : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGothamScreen(const FObjectInitializer& ObjectInitializer);

	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual void NativeConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Screens that must be answered (e.g. confirmations that block) can turn back off. */
	bool bCanDismissWithBack = true;

	/** Set by subclasses to the widget that should receive focus when the screen opens. */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> DefaultFocus;

	// Small builders so screens stay short and consistent.
	UTextBlock* MakeTitle(const FText& Text) const;
	UGothamButton* AddButton(UVerticalBox* Parent, const FText& Label) const;
	/** "[Enter/A] Select   [Esc/B] Back" prompt row. */
	class UHorizontalBox* MakeHintBar(const FText& AcceptLabel, const FText& BackLabel) const;
};
