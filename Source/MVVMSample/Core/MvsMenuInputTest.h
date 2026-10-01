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
 * Runs as the automation test Mvs.Functional.MenuInput (Scripts/run_tests.py's game pass), or as the dev aid
 * -MvsMenuInputTest, which logs PASS / FAIL per rule and quits.
 */
class MVVMSAMPLE_API FMvsMenuInputTest
{
public:
	/** The rules as a script, not started. Each result goes to Reporter; the last step logs the totals. */
	static TSharedPtr<FMvsScript> Build(AMvsPlayerController* Controller, FMvsScript::FReporter Reporter);
	/** The dev aid: build, start, log each rule to LogMvsMenuTest, quit when done. */
	static void Start(AMvsPlayerController* Controller);
};
