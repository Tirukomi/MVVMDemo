// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "GadgetViewModels.generated.h"

/** Presentation state for one gadget slot. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UGadgetSlotViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetDefinition(const FText& InName, const FText& InHotkey, const FLinearColor& InTint);

	/** Remaining and total seconds; derives percent and ready state. */
	void SetCooldown(float InRemaining, float InTotal);

	const FText& GetDisplayName() const { return DisplayName; }
	const FText& GetHotkey() const { return Hotkey; }
	FLinearColor GetTint() const { return Tint; }
	float GetCooldownRemaining() const { return CooldownRemaining; }
	float GetCooldownPercent() const { return CooldownPercent; }
	bool GetIsReady() const { return bIsReady; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText Hotkey;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float CooldownRemaining = 0.f;

	/** 0 = ready, 1 = just used. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float CooldownPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsReady, meta = (AllowPrivateAccess = "true"))
	bool bIsReady = true;
};

/** Owns the slot view models so the HUD can build one entry per slot. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UGadgetBarViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Rebuilds the slots; a view listening to Slots re-creates its entries. */
	void SetSlotCount(int32 Count);

	UGadgetSlotViewModel* GetSlot(int32 Index) const;
	const TArray<TObjectPtr<UGadgetSlotViewModel>>& GetSlots() const { return Slots; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UGadgetSlotViewModel>> Slots;
};
