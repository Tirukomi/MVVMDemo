// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "ConfirmModalScreen.generated.h"

class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnConfirmResult, bool /*bConfirmed*/);

/** Yes/No modal. Focus starts on "No" so an accidental Enter never confirms a destructive action. */
UCLASS()
class MVVMSAMPLE_API UConfirmModalScreen : public UGothamScreen
{
	GENERATED_BODY()

public:
	void Setup(const FText& InTitle, const FText& InBody, FOnConfirmResult InCallback);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnDeactivated() override;

private:
	void Finish(bool bConfirmed);

	FOnConfirmResult Callback;
	bool bAnswered = false;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;
};
