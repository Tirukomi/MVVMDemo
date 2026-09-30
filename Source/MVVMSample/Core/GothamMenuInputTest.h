// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AGothamPlayerController;

/**
 * Dev aid (-GothamMenuInputTest): drives the menus through Slate's own input path (in-engine key and mouse events,
 * never OS input) and logs PASS / FAIL for each rule, then quits. Covers what unit tests cannot: focus, hit-testing
 * and routing. Rules: the key that opens a screen closes it; clicking a prompt does what its key does.
 */
class FGothamMenuInputTest
{
public:
	static void Start(AGothamPlayerController* Controller);
};
