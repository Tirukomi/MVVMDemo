// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/GothamBindings.h"
#include "MVVMViewModelBase.h"
#include "ControlsViewModel.generated.h"

class IGothamBindingStore;

/**
 * Presentation state and commands for the controls screen. Holds a snapshot of the current bindings, validates rebind
 * requests (allowed key for the slot, swap on conflict) and applies them through a binding store
 * (IGothamBindingStore: Enhanced Input in the game, memory in tests). The screen only shows the result.
 */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UControlsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Where the bindings are read from and written to. Reads the current bindings. */
	void SetStore(TSharedPtr<IGothamBindingStore> InStore);

	/** Re-reads the bindings from the store (after something else changed them). */
	void Refresh();

	/**
	 * Validates a rebind and applies it (swapping keys on a conflict). Returns false (and sets StatusText) if the key is
	 * not allowed for the slot.
	 */
	bool RequestRebind(FName Name, int32 Slot, const FKey& NewKey);

	/** Puts every binding back to its default key. */
	void ResetToDefaults();

	void SetStatus(const FText& InStatus);

	FKey GetKey(FName Name, int32 Slot) const;
	const TArray<FGothamBindingSlot>& GetSnapshot() const { return Snapshot; }
	int32 GetRevision() const { return Revision; }
	const FText& GetStatusText() const { return StatusText; }

protected:
	/** Bumped whenever the snapshot changes, so rows know to re-read their keys. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 Revision = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText StatusText;

private:
	void SetSnapshot(TArray<FGothamBindingSlot> InSnapshot);

	TSharedPtr<IGothamBindingStore> Store;
	TArray<FGothamBindingSlot> Snapshot;
};
