// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "CommonButtonBase.h"
#include "UI/Widgets/MvsAcceptable.h"
#include "UI/Style/MvsStyle.h"
#include "MvsButton.generated.h"

class UMvsPanel;
class UTextBlock;

/**
 * Common UI's brushes are all "no draw": the button draws its own chamfered panel (UMvsPanel) so every state comes
 * from palette tokens and follows high contrast. The style class stays so Common UI's sounds and padding hooks work.
 */
UCLASS()
class MVVMSAMPLE_API UMvsButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UMvsButtonStyle();
};

/** What a button is for; decides its shape and how focus shows. */
UENUM()
enum class EMvsButtonKind : uint8
{
	/** Chamfered panel; focus adds an accent edge, accent bar and glow. */
	Standard,
	/** Big left-aligned label with no panel: a UMvsMenuList's sliding highlight shows focus instead. */
	MenuItem,
	/** Settings tab: label with an accent underline when selected. Not focusable (switched with the shoulders). */
	Tab,
	/** Like Standard, with the danger colour for destructive actions. */
	Danger,
};

/**
 * Standard menu button: a label on a Common UI button. Hovering with the mouse moves focus to it, so the mouse and a
 * gamepad always agree on which item is current (and the menu highlight follows either).
 */
UCLASS()
class MVVMSAMPLE_API UMvsButton : public UCommonButtonBase, public IMvsAcceptable
{
	GENERATED_BODY()

public:
	UMvsButton(const FObjectInitializer& ObjectInitializer);

	/** The click Enter would make (IMvsAcceptable). */
	virtual void Accept() override { HandleButtonClicked(); }

	void SetLabel(const FText& InLabel);
	void SetKind(EMvsButtonKind InKind);

protected:
	virtual bool Initialize() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual void HandleFocusReceived() override;
	virtual void HandleFocusLost() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnPressed() override;
	virtual void NativeOnReleased() override;
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnEnabled() override;
	virtual void NativeOnDisabled() override;

private:
	/** Recomputes panel and text colours from the current state and palette. */
	void ApplyState();
	void ApplyKind();

	UPROPERTY(Transient)
	TObjectPtr<UMvsPanel> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	FText PendingLabel;
	EMvsButtonKind Kind = EMvsButtonKind::Standard;
	bool bFocused = false;
	bool bHoveredNow = false;
	bool bPressedNow = false;
	FMvsSettingsListener SettingsListener;
};
