// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GothamViewModelSubsystem.generated.h"

class AGothamCharacter;
class UComboComponent;
class UComboViewModel;
class UGadgetBarViewModel;
class UGadgetComponent;
class UHealthComponent;
class UPlayerVitalsViewModel;

/**
 * Owns the HUD view models for one local player and wires gameplay components into them.
 * This is the only place that knows about both sides; components and widgets never see each other.
 */
UCLASS()
class MVVMSAMPLE_API UGothamViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Subscribes to the character's components (or unsubscribes when null) and pushes initial state. */
	void BindToCharacter(AGothamCharacter* Character);

	UPlayerVitalsViewModel* GetVitals() const { return Vitals; }
	UGadgetBarViewModel* GetGadgetBar() const { return GadgetBar; }
	UComboViewModel* GetCombo() const { return Combo; }

	/** Used by the view-model resolver to hand a view model to a widget by class. */
	UObject* FindViewModel(const UClass* ViewModelClass) const;

private:
	void Unbind();

	void HandleHealth(float Health, float MaxHealth);
	void HandleGadgetCooldown(int32 Slot, float Remaining, float Total);
	void HandleCombo(int32 Hits, float Multiplier, float DecayAlpha);

	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> Vitals;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBar;

	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> Combo;

	TWeakObjectPtr<UHealthComponent> BoundHealth;
	TWeakObjectPtr<UGadgetComponent> BoundGadgets;
	TWeakObjectPtr<UComboComponent> BoundCombo;

	FDelegateHandle HealthHandle;
	FDelegateHandle GadgetHandle;
	FDelegateHandle ComboHandle;
};
