// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "Components/TileView.h"
#include "ClueEntryWidget.generated.h"

class UClueEntryViewModel;
class UGothamPanel;
class UImage;
class UTextBlock;

/**
 * One evidence-board tile: a thumbnail with its case number and title. The tile view pools these: it creates only
 * enough to fill the viewport and rebinds them to different view models while scrolling, so tiles must rebind
 * cleanly and cancel any in-flight thumbnail load. Selection (which follows gamepad / keyboard navigation) lights
 * the tile's edge; the case file's detail pane shows the selected clue.
 */
UCLASS()
class MVVMSAMPLE_API UClueEntryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	static constexpr float TileWidth = 188.f;
	static constexpr float TileHeight = 172.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
	virtual void NativeConstruct() override;
	virtual void NativeOnEntryReleased() override;
	virtual void NativeDestruct() override;

private:
	void Bind(UClueEntryViewModel* InViewModel);
	void Refresh();
	void ApplySelection();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	bool bSelected = false;

	UPROPERTY(Transient)
	TObjectPtr<UClueEntryViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Thumbnail;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UnknownMark;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CaseNumber;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;
};

/** Tile view whose entry class is set from code (no designer asset to point at it). */
UCLASS()
class MVVMSAMPLE_API UGothamClueTileView : public UTileView
{
	GENERATED_BODY()

public:
	void SetEntryClass(TSubclassOf<UUserWidget> InClass) { EntryWidgetClass = InClass; }
};

/** Case number shown on tiles and in the detail pane: "No. 007". Pure, for tests. */
MVVMSAMPLE_API FText GothamCaseNumber(int32 Index);
