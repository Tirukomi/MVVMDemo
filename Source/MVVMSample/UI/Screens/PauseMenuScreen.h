// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/MvsScreen.h"
#include "PauseMenuScreen.generated.h"

class UClueListViewModel;
class UMvsPanel;
class UObjectivesViewModel;
class UTextBlock;

/**
 * Pause menu: Resume / Case file / Settings / Quit (quit asks for confirmation) in a left column with the sliding
 * highlight, and a status panel (objective, evidence found) on the right. Pauses the game while open.
 */
UCLASS()
class MVVMSAMPLE_API UPauseMenuScreen : public UMvsScreen
{
	GENERATED_BODY()

public:
	/** Opens the quit confirmation, as the Quit item does. */
	void RequestQuit() { OnQuit(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnClosed() override;
	virtual void ApplyTheme(const FMvsTheme& Theme) override;

private:
	void RefreshStatus();
	void OnStatusChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshStatus(); }
	void OnResume();
	void OnCaseFile();
	void OnSettings();
	void OnQuit();
	void OnQuitConfirmed(bool bConfirmed);

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> Objectives;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UMvsPanel> StatusPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> EvidenceText;
};
