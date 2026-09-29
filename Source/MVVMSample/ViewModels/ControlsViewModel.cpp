// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/ControlsViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Controls"

void UControlsViewModel::SetSnapshot(const TArray<FGothamBindingSlot>& InSnapshot)
{
	Snapshot = InSnapshot;
	UE_MVVM_SET_PROPERTY_VALUE(Revision, Revision + 1);
}

FKey UControlsViewModel::GetKey(FName Name, int32 Slot) const
{
	const FGothamBindingSlot* Found = Snapshot.FindByPredicate([&](const FGothamBindingSlot& S) { return S.Name == Name && S.Slot == Slot; });
	return Found ? Found->Key : FKey();
}

void UControlsViewModel::SetStatus(const FText& InStatus)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, InStatus);
}

bool UControlsViewModel::RequestRebind(FName Name, int32 Slot, const FKey& NewKey)
{
	if (!GothamBindings::IsKeyAllowedForSlot(Slot, NewKey))
	{
		SetStatus(Slot == GothamBindings::GamepadSlot
			? LOCTEXT("BadPad", "That button cannot be used here. Choose a gamepad button.")
			: LOCTEXT("BadKey", "That key cannot be used here. Choose another key or mouse button."));
		return false;
	}

	const TArray<FGothamBindingChange> Changes = GothamBindings::PlanRebind(Snapshot, Name, Slot, NewKey);
	if (Changes.IsEmpty())
	{
		SetStatus(FText::GetEmpty());
		return true; // already bound to that key: nothing to do
	}

	SetStatus(Changes.Num() > 1 ? LOCTEXT("Swapped", "That key was already in use; the two actions swapped keys.") : FText::GetEmpty());
	OnChangesPlanned.Broadcast(Changes);
	return true;
}

#undef LOCTEXT_NAMESPACE
