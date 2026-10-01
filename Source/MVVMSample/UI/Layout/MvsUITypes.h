// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MvsUITypes.generated.h"

/** Z-ordered UI layers, lowest first. Each layer is an activatable-widget stack in the primary layout. */
UENUM(BlueprintType)
enum class EMvsUILayer : uint8
{
	Game,      // HUD
	GameMenu,  // in-world overlays over gameplay (the gadget wheel)
	Menu,      // pause, settings, key bindings, the case file
	Modal,     // confirmations
	Count UMETA(Hidden)
};

/**
 * Pure bookkeeping for "what is open in each layer" and whether that counts as a menu being open.
 * No UObjects or widgets, so the rules are unit-testable without a world.
 */
struct FMvsUIModeTracker
{
	void SetLayerOccupied(EMvsUILayer Layer, bool bOccupied)
	{
		Occupied[static_cast<int32>(Layer)] = bOccupied;
	}

	bool IsLayerOccupied(EMvsUILayer Layer) const
	{
		return Occupied[static_cast<int32>(Layer)];
	}

	/** A menu or modal is open (pause, settings, the case file, confirmations); the in-world GameMenu layer does not count. */
	bool IsMenuOpen() const
	{
		return IsLayerOccupied(EMvsUILayer::Menu) || IsLayerOccupied(EMvsUILayer::Modal);
	}

	/** Topmost occupied layer that back/cancel should dismiss, or Count if only the HUD is up. */
	EMvsUILayer GetTopDismissableLayer() const
	{
		for (int32 i = static_cast<int32>(EMvsUILayer::Modal); i > static_cast<int32>(EMvsUILayer::Game); --i)
		{
			if (Occupied[i])
			{
				return static_cast<EMvsUILayer>(i);
			}
		}
		return EMvsUILayer::Count;
	}

private:
	bool Occupied[static_cast<int32>(EMvsUILayer::Count)] = {};
};
