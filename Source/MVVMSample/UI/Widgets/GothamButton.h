// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "GothamButton.generated.h"

class UTextBlock;

/** Button visuals as a style class so a re-skin is a content change, not a code change. */
UCLASS()
class MVVMSAMPLE_API UGothamButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()

public:
	UGothamButtonStyle();
};

/** Standard menu button: a label on a Common UI button, with hover/press/focus states from the style. */
UCLASS()
class MVVMSAMPLE_API UGothamButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UGothamButton(const FObjectInitializer& ObjectInitializer);

	void SetLabel(const FText& InLabel);

protected:
	virtual bool Initialize() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;

	FText PendingLabel;
};
