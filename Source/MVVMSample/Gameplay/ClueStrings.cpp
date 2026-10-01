// Copyright IG. All Rights Reserved.

#include "Gameplay/ClueStrings.h"

#include "Internationalization/StringTableRegistry.h"

namespace MvsClueStrings
{
	void Register()
	{
		LOCTABLE_NEW("MvsClues", "Mvs.ClueTable");

		LOCTABLE_SETSTRING("MvsClues", "Ledger.Title", "Torn ledger page");
		LOCTABLE_SETSTRING("MvsClues", "Ledger.Body", "A page torn from a shipping ledger. Three crates never reached the docks.");

		LOCTABLE_SETSTRING("MvsClues", "Footprint.Title", "Muddy footprint");
		LOCTABLE_SETSTRING("MvsClues", "Footprint.Body", "A size 12 boot print, heading away from the warehouse.");

		LOCTABLE_SETSTRING("MvsClues", "Casing.Title", "Spent shell casing");
		LOCTABLE_SETSTRING("MvsClues", "Casing.Body", "9mm, fired recently. The rounding on the rim points to a custom load.");

		LOCTABLE_SETSTRING("MvsClues", "Keycard.Title", "Dropped keycard");
		LOCTABLE_SETSTRING("MvsClues", "Keycard.Body", "Maintenance access. Someone went back for it and did not find it.");

		LOCTABLE_SETSTRING("MvsClues", "Note.Title", "Crumpled note");
		LOCTABLE_SETSTRING("MvsClues", "Note.Body", "Half a phone number and the word 'midnight' underlined twice.");
	}
}
