// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/MvsBindings.h"

class ULocalPlayer;

/**
 * Where the player's key bindings live: read them, change them, reset them. Changes persist immediately. The controls
 * view model works through this, so it can apply rebinds itself and be tested with a store in memory.
 */
class IMvsBindingStore
{
public:
	virtual ~IMvsBindingStore() = default;

	/** Every rebindable (action, slot) and its current key. */
	virtual TArray<FMvsBindingSlot> GetBindings() const = 0;
	/** Assigns the keys and saves them. */
	virtual void Apply(const TArray<FMvsBindingChange>& Changes) = 0;
	/** Back to the default keys, saved. */
	virtual void ResetToDefaults() = 0;
};

namespace MvsBindings
{
	/** The store for Player's Enhanced Input user settings (key profile rows, saved with SaveSettings). */
	MVVMSAMPLE_API TSharedRef<IMvsBindingStore> MakeEnhancedInputStore(ULocalPlayer* Player);
}
