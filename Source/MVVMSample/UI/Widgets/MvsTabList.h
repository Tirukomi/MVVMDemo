// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonTabListWidgetBase.h"
#include "MvsTabList.generated.h"

class UHorizontalBox;

/** The tab pages' switcher: a short sideways slide between pages, none under reduced motion. */
UCLASS()
class MVVMSAMPLE_API UMvsSwitcher : public UCommonAnimatedSwitcher
{
	GENERATED_BODY()

public:
	UMvsSwitcher(const FObjectInitializer& ObjectInitializer);
};

/**
 * Common UI tab list with UMvsButton tabs laid out in a row between the "previous" and "next" prompts
 * (Q / E, LB / RB). The keys are Common UI's tab actions (UMvsUIInputData), bound while the list is on screen; tabs
 * are not focus stops, so up / down navigation stays inside the page.
 *
 * Common UI removes every tab when the list is destructed, which happens each time its screen closes; the list keeps
 * what AddTab registered and registers it again when it is constructed, on the tab it was showing.
 */
UCLASS()
class MVVMSAMPLE_API UMvsTabList : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

public:
	UMvsTabList(const FObjectInitializer& ObjectInitializer);

	/** Registers a tab page. The tab button is created by Common UI's pool, then labelled. */
	bool AddTab(FName TabId, const FText& Label, UWidget* Content);

	/** Selects the tab Direction steps away (wrapping). Returns true if the selection changed. */
	bool SelectRelative(int32 Direction);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;

private:
	void EnsureTree();
	bool Register(int32 Index);

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> TabBox;

	/** What AddTab registered, in order, to register again after a destruct. */
	TArray<FName> TabIds;
	TArray<FText> TabLabels;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> TabContents;
	/** The tab showing when the list was last destructed. */
	FName LastSelectedTab;
};
