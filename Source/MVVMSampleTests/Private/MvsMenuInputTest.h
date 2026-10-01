// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/MvsScript.h"

class AMvsPlayerController;

/**
 * The menu input rules, driven through Slate's own input path (in-engine key and mouse events, never OS input).
 * Covers what unit tests cannot: focus, hit-testing and routing. Rules: the key that opens a screen closes it;
 * clicking a prompt does what its key does; an open screen restyles live.
 *
 * Runs as the automation test Mvs.Functional.MenuInput (Scripts/run_tests.py's game pass). Part of the test module,
 * so none of it is in a Shipping build.
 */
class FMvsMenuInputTest
{
public:
	/** The rules as a script, not started. Each result goes to Reporter; the last step logs the totals. */
	static TSharedPtr<FMvsScript> Build(AMvsPlayerController* Controller, FMvsScript::FReporter Reporter);
};
