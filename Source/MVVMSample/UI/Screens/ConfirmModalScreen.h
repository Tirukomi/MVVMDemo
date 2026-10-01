// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/MvsScreen.h"
#include "ConfirmModalScreen.generated.h"

class UMvsButton;
class UMvsPanel;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnConfirmResult, bool /*bConfirmed*/);

/**
 * Yes/No modal in the chamfered panel style. Focus starts on "No" so an accidental Enter never confirms a destructive
 * action. Destructive confirmations get the danger accent and a "Caution" label, so the warning is never colour alone.
 */
UCLASS()
class MVVMSAMPLE_API UConfirmModalScreen : public UMvsScreen
{
	GENERATED_BODY()

public:
	void Setup(const FText& InTitle, const FText& InBody, FOnConfirmResult InCallback, bool bInDestructive = false);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnClosed() override;
	virtual void ApplyTheme() override;

private:
	void Finish(bool bConfirmed);

	FOnConfirmResult Callback;
	bool bAnswered = false;
	bool bDestructive = false;

	UPROPERTY(Transient)
	TObjectPtr<UMvsPanel> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CautionText;

	UPROPERTY(Transient)
	TObjectPtr<UMvsButton> YesButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;
};
