// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/GothamBindings.h"

class ULocalPlayer;

/**
 * Where the player's key bindings live: read them, change them, reset them. Changes persist immediately. The controls
 * view model works through this, so it can apply rebinds itself and be tested with a store in memory.
 */
class IGothamBindingStore
{
public:
	virtual ~IGothamBindingStore() = default;

	/** Every rebindable (action, slot) and its current key. */
	virtual TArray<FGothamBindingSlot> GetBindings() const = 0;
	/** Assigns the keys and saves them. */
	virtual void Apply(const TArray<FGothamBindingChange>& Changes) = 0;
	/** Back to the default keys, saved. */
	virtual void ResetToDefaults() = 0;
};

namespace GothamBindings
{
	/** The store for Player's Enhanced Input user settings (key profile rows, saved with SaveSettings). */
	MVVMSAMPLE_API TSharedRef<IGothamBindingStore> MakeEnhancedInputStore(ULocalPlayer* Player);
}
