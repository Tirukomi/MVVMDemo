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
 *
 * The designer path (second review 11, Docs/DesignerGuide.md): a Widget Blueprint parented to this class authors the
 * status panel's content (its whole widget tree goes into the panel) and fills it through MVVM View Bindings with
 * UMvsViewModelResolver; the frame, the menu and the panel stay code-built. Without a designer tree the content is
 * code-built too.
 */
UCLASS()
class MVVMSAMPLE_API UPauseMenuScreen : public UMvsScreen
{
	GENERATED_BODY()

public:
	/** Opens the quit confirmation, as the Quit item does. */
	void RequestQuit() { OnQuit(); }

	/** The status panel (its content is the designer's tree, or code-built text). */
	const UMvsPanel* GetStatusPanel() const { return StatusPanel; }

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

	/**
	 * The captions above the objective and the evidence. A designer's tree names its text widgets ObjectiveLabel and
	 * EvidenceLabel and the screen fills them, so they keep the game's translations (a text typed into a Widget
	 * Blueprint gets a key of its own, which no translation has).
	 */
	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ObjectiveLabel;

	UPROPERTY(Transient, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EvidenceLabel;
};
