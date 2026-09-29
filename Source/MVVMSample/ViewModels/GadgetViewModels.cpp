// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GadgetViewModels.h"

void UGadgetSlotViewModel::SetDefinition(const FText& InName, const FText& InHotkey, const FLinearColor& InTint, int32 InIconIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(IconIndex, InIconIndex);
	UE_MVVM_SET_PROPERTY_VALUE(DisplayName, InName);
	UE_MVVM_SET_PROPERTY_VALUE(Hotkey, InHotkey);
	UE_MVVM_SET_PROPERTY_VALUE(Tint, InTint);
}

void UGadgetSlotViewModel::SetCooldown(float InRemaining, float InTotal)
{
	const float Remaining = FMath::Max(0.f, InRemaining);
	const float Percent = InTotal > 0.f ? FMath::Clamp(Remaining / InTotal, 0.f, 1.f) : 0.f;

	UE_MVVM_SET_PROPERTY_VALUE(CooldownRemaining, Remaining);
	UE_MVVM_SET_PROPERTY_VALUE(CooldownPercent, Percent);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsReady, Remaining <= 0.f);
}

void UGadgetBarViewModel::SetSlotCount(int32 Count)
{
	if (Slots.Num() == Count)
	{
		return;
	}

	TArray<TObjectPtr<UGadgetSlotViewModel>> NewSlots;
	for (int32 i = 0; i < Count; ++i)
	{
		NewSlots.Add(Slots.IsValidIndex(i) ? Slots[i].Get() : NewObject<UGadgetSlotViewModel>(this));
	}
	Slots = MoveTemp(NewSlots);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
}

void UGadgetBarViewModel::SetSelectedIndex(int32 InIndex)
{
	if (Slots.IsValidIndex(InIndex))
	{
		UE_MVVM_SET_PROPERTY_VALUE(SelectedIndex, InIndex);
	}
}

UGadgetSlotViewModel* UGadgetBarViewModel::GetSlot(int32 Index) const
{
	return Slots.IsValidIndex(Index) ? Slots[Index].Get() : nullptr;
}
