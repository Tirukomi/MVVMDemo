// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/GothamBindings.h"
#include "MVVMViewModelBase.h"
#include "ControlsViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBindingChangesPlanned, const TArray<FGothamBindingChange>&);

/**
 * Presentation state for the controls screen. Holds a snapshot of the current bindings, validates rebind requests
 * (allowed key for the slot, swap on conflict) and reports what to apply. It never touches Enhanced Input itself.
 */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UControlsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Fired when a valid rebind was planned; the owner applies the changes and calls SetSnapshot with the result. */
	FOnBindingChangesPlanned OnChangesPlanned;

	void SetSnapshot(const TArray<FGothamBindingSlot>& InSnapshot);

	/** Validates and plans a rebind. Returns false (and sets StatusText) if the key is not allowed for the slot. */
	bool RequestRebind(FName Name, int32 Slot, const FKey& NewKey);

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
	TArray<FGothamBindingSlot> Snapshot;
};
