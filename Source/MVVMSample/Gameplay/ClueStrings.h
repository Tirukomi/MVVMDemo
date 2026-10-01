// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The clue log's text lives in a string table (id "MvsClues") registered at module start-up. Clue data assets
 * reference entries by key, so designers edit one table and the localization gatherer picks every string up.
 */
namespace MvsClueStrings
{
	inline constexpr const TCHAR* TableId = TEXT("MvsClues");

	void Register();
}
