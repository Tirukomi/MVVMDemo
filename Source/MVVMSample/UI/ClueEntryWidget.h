// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/ListView.h"
#include "ClueEntryWidget.generated.h"

class UBorder;
class UClueEntryViewModel;
class UImage;
class UTextBlock;

/**
 * One clue-log row. The list view pools these: it creates only enough to fill the viewport and rebinds them to
 * different view models while scrolling, so rows must rebind cleanly and cancel any in-flight thumbnail load.
 */
UCLASS()
class MVVMSAMPLE_API UClueEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeDestruct() override;

private:
	void Bind(UClueEntryViewModel* InViewModel);
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UClueEntryViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Thumbnail;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;
};

/** List view whose entry class is set from code (no designer asset to point at it). */
UCLASS()
class MVVMSAMPLE_API UGothamClueListView : public UListView
{
	GENERATED_BODY()

public:
	void SetEntryClass(TSubclassOf<UUserWidget> InClass) { EntryWidgetClass = InClass; }
};
