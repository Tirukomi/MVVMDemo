// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "GothamMenuList.generated.h"

class SGothamHighlight;
class UVerticalBox;
class UVerticalBoxSlot;

/** UMG wrapper for the highlight bar, so it can sit in a widget tree behind the items. */
UCLASS()
class MVVMSAMPLE_API UGothamHighlightBar : public UWidget
{
	GENERATED_BODY()

public:
	TSharedPtr<SGothamHighlight> GetSlate() const { return MyHighlight; }
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SGothamHighlight> MyHighlight;
};

/**
 * A vertical menu with a sliding highlight bar behind the current item. Event-driven: the list watches focus changes
 * inside itself (NativeOnFocusChanging) and moves the bar to whichever item is on the new focus path, so it works for
 * gamepad, keyboard and mouse (buttons move focus on hover) and for any focusable item type.
 */
UCLASS()
class MVVMSAMPLE_API UGothamMenuList : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Adds an item below the others. The item, or something inside it, should be focusable. */
	UVerticalBoxSlot* AddItem(UWidget* Item);
	const TArray<TObjectPtr<UWidget>>& GetItems() const { return Items; }

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCurrentItemChanged, UWidget*);
	FOnCurrentItemChanged OnCurrentItemChanged;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;

private:
	void ApplyLook();

	UPROPERTY(Transient)
	TObjectPtr<UGothamHighlightBar> Highlight;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> Box;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> Items;

	TWeakObjectPtr<UWidget> CurrentItem;
	FGothamSettingsListener SettingsListener;
};
