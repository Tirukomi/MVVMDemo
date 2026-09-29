// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The clue log's text lives in a string table (id "GothamClues") registered at module start-up. Clue data assets
 * reference entries by key, so designers edit one table and the localization gatherer picks every string up.
 */
namespace GothamClueStrings
{
	inline constexpr const TCHAR* TableId = TEXT("GothamClues");

	void Register();
}
