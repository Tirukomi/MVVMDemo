// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/ThreatViewModel.h"

void UThreatViewModel::SetThreats(const TArray<FGothamThreatSnapshot>& InThreats)
{
	Threats = InThreats;
	int32 Warnings = 0;
	for (const FGothamThreatSnapshot& Threat : Threats)
	{
		Warnings += Threat.State == EGothamThugState::Warning ? 1 : 0;
	}
	UE_MVVM_SET_PROPERTY_VALUE(ThreatCount, Threats.Num());
	UE_MVVM_SET_PROPERTY_VALUE(WarningCount, Warnings);
}
