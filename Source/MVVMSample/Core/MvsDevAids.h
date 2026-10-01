// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

class AMvsPlayerController;
class UMvsUISubsystem;

/**
 * Dev aids for headless verification, enabled by command-line flags (never by default, compiled out of Shipping).
 * Screenshot flags save a screenshot after -MvsShotDelay seconds (default 4). See README for the full list.
 *   -MvsOpenPause     opens the pause menu
 *   -MvsOpenQuit      opens the pause menu, then its (destructive) quit confirmation
 *   -MvsMenuInputTest toggle-key and clickable-prompt checks through Slate input; logs PASS / FAIL, then quits
 *   -MvsPerf=<label>  runs the UI performance harness, writes Saved/Perf/<label>.md, then quits
 *   -MvsQuitAfterLoad quits once the level and HUD are up (the perf gate's warm-up)
 *   -MvsCombatDemo    a thug in view and one behind telegraph at once (prompt + arrow), with a 10-hit combo
 *   -MvsHudDemo       a recent hit, a live combo and a gadget recharging
 *   -MvsOpenWheel     opens the gadget wheel, hovers a segment and builds a combo
 *   -MvsForensic     enters forensic mode and scans the nearest clue (-MvsForensicReveal / -Analyse: mid-effect)
 *   -MvsClueLog[=N]   scans a clue, opens the case file, optionally with N extra fake clues
 *   -MvsOpenSettings, -MvsOpenControls, -MvsRebindDemo, -MvsCycleLanguage, -MvsShotName=<name>
 * Thugs never start attacks on their own during these runs (except -MvsCombatDemo's forced ones).
 */
namespace MvsDevAids
{
	/** Called once the HUD is up. Does nothing unless a dev flag is on the command line. */
	void Run(AMvsPlayerController* Controller, UMvsUISubsystem* UI);
}

#endif
