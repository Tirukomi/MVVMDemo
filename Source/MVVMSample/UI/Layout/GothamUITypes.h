// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GothamUITypes.generated.h"

/** Z-ordered UI layers, lowest first. Each layer is an activatable-widget stack in the primary layout. */
UENUM(BlueprintType)
enum class EGothamUILayer : uint8
{
	Game,      // HUD
	GameMenu,  // in-world overlays over gameplay (the gadget wheel)
	Menu,      // pause, settings, inventory
	Modal,     // confirmations
	Count UMETA(Hidden)
};

/**
 * Pure bookkeeping for "what is open in each layer" and whether that counts as a menu being open.
 * No UObjects or widgets, so the rules are unit-testable without a world.
 */
struct FGothamUIModeTracker
{
	void SetLayerOccupied(EGothamUILayer Layer, bool bOccupied)
	{
		Occupied[static_cast<int32>(Layer)] = bOccupied;
	}

	bool IsLayerOccupied(EGothamUILayer Layer) const
	{
		return Occupied[static_cast<int32>(Layer)];
	}

	/** A menu or modal is open (pause, settings, the case file, confirmations); the in-world GameMenu layer does not count. */
	bool IsMenuOpen() const
	{
		return IsLayerOccupied(EGothamUILayer::Menu) || IsLayerOccupied(EGothamUILayer::Modal);
	}

	/** Topmost occupied layer that back/cancel should dismiss, or Count if only the HUD is up. */
	EGothamUILayer GetTopDismissableLayer() const
	{
		for (int32 i = static_cast<int32>(EGothamUILayer::Modal); i > static_cast<int32>(EGothamUILayer::Game); --i)
		{
			if (Occupied[i])
			{
				return static_cast<EGothamUILayer>(i);
			}
		}
		return EGothamUILayer::Count;
	}

private:
	bool Occupied[static_cast<int32>(EGothamUILayer::Count)] = {};
};
