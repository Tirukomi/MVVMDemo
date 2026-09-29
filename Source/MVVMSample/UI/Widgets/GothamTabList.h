// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonTabListWidgetBase.h"
#include "GothamTabList.generated.h"

class UHorizontalBox;

/** The tab pages' switcher: a short sideways slide between pages, none under reduced motion. */
UCLASS()
class MVVMSAMPLE_API UGothamSwitcher : public UCommonAnimatedSwitcher
{
	GENERATED_BODY()

public:
	UGothamSwitcher(const FObjectInitializer& ObjectInitializer);
};

/**
 * Common UI tab list with UGothamButton tabs laid out in a row between the "previous" and "next" prompts
 * (Q / E, LB / RB). The owning screen forwards those keys to SelectRelative: tabs are not focus stops, so up / down
 * navigation stays inside the page.
 */
UCLASS()
class MVVMSAMPLE_API UGothamTabList : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	/** Registers a tab page. The tab button is created by Common UI's pool, then labelled. */
	bool AddTab(FName TabId, const FText& Label, UWidget* Content);

	/** Selects the tab Direction steps away (wrapping). Returns true if the selection changed. */
	bool SelectRelative(int32 Direction);

	/** True for the keys that step tabs: Q / E and the shoulder buttons. Out Direction is -1 or +1. */
	static bool IsTabKey(const FKey& Key, int32& OutDirection);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

private:
	void EnsureTree();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TabBox;
};
