// Copyright IG. All Rights Reserved.

#include "ViewModels/ThreatViewModel.h"

void UThreatViewModel::SetThreats(const TArray<FMvsThreatSnapshot>& InThreats)
{
	Threats = InThreats;
	int32 Warnings = 0;
	for (const FMvsThreatSnapshot& Threat : Threats)
	{
		Warnings += Threat.State == EMvsThugState::Warning ? 1 : 0;
	}
	UE_MVVM_SET_PROPERTY_VALUE(ThreatCount, Threats.Num());
	UE_MVVM_SET_PROPERTY_VALUE(WarningCount, Warnings);
}
