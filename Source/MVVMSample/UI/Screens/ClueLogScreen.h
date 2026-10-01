// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "ClueLogScreen.generated.h"

class UClueEntryViewModel;
class UClueListViewModel;
class UGothamClueTileView;
class UGothamPanel;
class UImage;
class UTextBlock;

/**
 * Case file as an evidence board: every clue in the level as a tile in a pooled, virtualised tile view, and a detail
 * pane for the selected one (large thumbnail, case number, title, notes). Undiscovered clues show as "???".
 */
UCLASS()
class MVVMSAMPLE_API UClueLogScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void ApplyTheme() override;

private:
	void RefreshList();
	void OnClueListChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshList(); }
	void OnSelectionChanged(UObject* Item);
	void BindDetail(UClueEntryViewModel* Entry);
	void RefreshDetail();
	void OnDetailChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshDetail(); }

	UPROPERTY(Transient)
	TObjectPtr<UGothamClueTileView> TileView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Summary;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UClueEntryViewModel> DetailEntry;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> DetailPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DetailImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailNumber;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailStatus;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailBody;
};
