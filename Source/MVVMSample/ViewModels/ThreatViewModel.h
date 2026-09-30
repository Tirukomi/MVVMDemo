// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/ThreatTypes.h"
#include "MVVMViewModelBase.h"
#include "ThreatViewModel.generated.h"

/**
 * Hostiles for the HUD. The counts are field-notify (they change rarely and switch the threat layer on and off); the
 * per-frame positions are a plain array the threat layer reads while it is drawing, like the clue markers do.
 */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UThreatViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetThreats(const TArray<FGothamThreatSnapshot>& InThreats);

	const TArray<FGothamThreatSnapshot>& GetThreats() const { return Threats; }
	int32 GetThreatCount() const { return ThreatCount; }
	int32 GetWarningCount() const { return WarningCount; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 ThreatCount = 0;

	/** Thugs telegraphing an attack right now (the counter window is open). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 WarningCount = 0;

private:
	TArray<FGothamThreatSnapshot> Threats;
};
