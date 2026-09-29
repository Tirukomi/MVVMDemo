// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AGothamPlayerController;

/**
 * Dev-only UI performance harness (enable with -GothamPerf=<label>). Steps through fixed UI scenarios, samples
 * frame and game-thread time for each, counts live widgets and memory, then writes Saved/Perf/<label>.md and exits.
 * Numbers are for before/after comparison on one machine, not absolute budgets.
 */
class FGothamPerfHarness
{
public:
	/** Starts the run; the harness owns its own lifetime and quits the game when finished. */
	static void Start(AGothamPlayerController* Controller, const FString& Label);
};

#endif // !UE_BUILD_SHIPPING
