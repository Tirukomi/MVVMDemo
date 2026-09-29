// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "ClueLogScreen.generated.h"

class UClueListViewModel;
class UGothamClueListView;
class UTextBlock;

/** Case file: every clue in the level in a pooled, virtualised list. Undiscovered clues show as "???". */
UCLASS()
class MVVMSAMPLE_API UClueLogScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RefreshList();
	void OnClueListChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshList(); }

	UPROPERTY(Transient)
	TObjectPtr<UGothamClueListView> ListView;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Summary;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;
};
