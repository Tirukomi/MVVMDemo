// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AGothamPlayerController;
class UGothamUISubsystem;

/**
 * Dev aids for headless verification, enabled by command-line flags (never by default, compiled out of Shipping).
 * Screenshot flags save a screenshot after -GothamShotDelay seconds (default 4). See README for the full list.
 *   -GothamOpenPause     opens the pause menu
 *   -GothamOpenQuit      opens the pause menu, then its (destructive) quit confirmation
 *   -GothamMenuInputTest toggle-key and clickable-prompt checks through Slate input; logs PASS / FAIL, then quits
 *   -GothamPerf=<label>  runs the UI performance harness, writes Saved/Perf/<label>.md, then quits
 *   -GothamQuitAfterLoad quits once the level and HUD are up (the perf gate's warm-up)
 *   -GothamCombatDemo    a thug in view and one behind telegraph at once (prompt + arrow), with a 10-hit combo
 *   -GothamHudDemo       a recent hit, a live combo and a gadget recharging
 *   -GothamOpenWheel     opens the gadget wheel, hovers a segment and builds a combo
 *   -GothamDetective     enters detective mode and scans the nearest clue (-GothamDetectiveReveal / -Analyse: mid-effect)
 *   -GothamClueLog[=N]   scans a clue, opens the case file, optionally with N extra fake clues
 *   -GothamOpenSettings, -GothamOpenControls, -GothamRebindDemo, -GothamCycleLanguage, -GothamShotName=<name>
 * Thugs never start attacks on their own during these runs (except -GothamCombatDemo's forced ones).
 */
namespace GothamDevAids
{
	/** Called once the HUD is up. Does nothing unless a dev flag is on the command line. */
	void Run(AGothamPlayerController* Controller, UGothamUISubsystem* UI);
}

#endif
