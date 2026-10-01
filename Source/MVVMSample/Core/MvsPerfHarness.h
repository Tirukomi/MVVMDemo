// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AMvsPlayerController;

/**
 * Dev-only UI performance harness (enable with -MvsPerf=<label>). Steps through fixed UI scenarios, samples
 * frame and game-thread time for each, counts live widgets and memory, then writes Saved/Perf/<label>.md and exits.
 * Numbers are for before/after comparison on one machine, not absolute budgets.
 */
class FMvsPerfHarness
{
public:
	/** Starts the run; the harness owns its own lifetime and quits the game when finished. */
	static void Start(AMvsPlayerController* Controller, const FString& Label);
};

#endif // !UE_BUILD_SHIPPING
