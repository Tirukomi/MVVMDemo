// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonInputBaseTypes.h"
#include "MvsUIInput.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * Common UI's input data (Config/DefaultGame.ini, CommonInputSettings.InputData): the menu actions every screen shares.
 * Common UI uses the accept and back actions itself (the back handler, button clicks); the tab actions drive tab lists.
 *
 * The actions are built in code, like the gameplay ones, so the project needs no input assets. Their keys come from
 * BuildMappingContext, which the player controller adds on top of the gameplay context and never removes: Common UI
 * finds an action's keys through Enhanced Input's active mappings, and its Menu input mode keeps those keys from
 * reaching the game while a menu is open.
 */
UCLASS()
class MVVMSAMPLE_API UMvsUIInputData : public UCommonUIInputData
{
	GENERATED_BODY()

public:
	UMvsUIInputData();

	static const UMvsUIInputData& Get() { return *GetDefault<UMvsUIInputData>(); }

	const UInputAction* GetAcceptAction() const { return EnhancedInputClickAction; }
	const UInputAction* GetBackAction() const { return EnhancedInputBackAction; }
	UInputAction* GetPreviousTabAction() const { return PreviousTabAction; }
	UInputAction* GetNextTabAction() const { return NextTabAction; }

	/**
	 * The menu keys. Accept and back use the platform's virtual gamepad keys, so platforms that swap the face buttons
	 * get the swap. None of the actions consume their keys: Q and E also open the wheel and scan in gameplay.
	 */
	UInputMappingContext* BuildMappingContext(UObject* Outer) const;

	/** The menu context sits above gameplay so its keys are never shadowed by a gameplay action using the same key. */
	static constexpr int32 MappingPriority = 1;

private:
	UPROPERTY()
	TObjectPtr<UInputAction> PreviousTabAction;

	UPROPERTY()
	TObjectPtr<UInputAction> NextTabAction;
};
