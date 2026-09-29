// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ClueStrings.h"

#include "Internationalization/StringTableRegistry.h"

namespace GothamClueStrings
{
	void Register()
	{
		LOCTABLE_NEW("GothamClues", "Gotham.ClueTable");

		LOCTABLE_SETSTRING("GothamClues", "Ledger.Title", "Torn ledger page");
		LOCTABLE_SETSTRING("GothamClues", "Ledger.Body", "A page torn from a shipping ledger. Three crates never reached the docks.");

		LOCTABLE_SETSTRING("GothamClues", "Footprint.Title", "Muddy footprint");
		LOCTABLE_SETSTRING("GothamClues", "Footprint.Body", "A size 12 boot print, heading away from the warehouse.");

		LOCTABLE_SETSTRING("GothamClues", "Casing.Title", "Spent shell casing");
		LOCTABLE_SETSTRING("GothamClues", "Casing.Body", "9mm, fired recently. The rounding on the rim points to a custom load.");

		LOCTABLE_SETSTRING("GothamClues", "Keycard.Title", "Dropped keycard");
		LOCTABLE_SETSTRING("GothamClues", "Keycard.Body", "Maintenance access. Someone went back for it and did not find it.");

		LOCTABLE_SETSTRING("GothamClues", "Note.Title", "Crumpled note");
		LOCTABLE_SETSTRING("GothamClues", "Note.Body", "Half a phone number and the word 'midnight' underlined twice.");
	}
}
