// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UObject/WeakObjectPtr.h"

class UGothamSettingsSubsystem;
struct FGothamSettingsData;

/**
 * A subscription to UGothamSettingsSubsystem::OnSettingsChanged that cleans up after itself. Keep one as a member:
 *
 *     FGothamSettingsListener SettingsListener;
 *     ... NativeConstruct:  SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyStyle(); });
 *     ... NativeDestruct:   SettingsListener.Reset();   (or let the destructor do it)
 *
 * The callback is scoped to Owner (it never runs once Owner is gone), and the subsystem is held weakly, so the
 * order in which the owner and the game instance shut down does not matter. Binding again replaces the old
 * subscription, so a widget that is constructed twice never ends up subscribed twice.
 */
class MVVMSAMPLE_API FGothamSettingsListener
{
public:
	using FCallback = TFunction<void(const FGothamSettingsData&)>;

	FGothamSettingsListener() = default;
	~FGothamSettingsListener() { Reset(); }
	FGothamSettingsListener(const FGothamSettingsListener&) = delete;
	FGothamSettingsListener& operator=(const FGothamSettingsListener&) = delete;

	/** Subscribes through Owner's world. Returns false if there is no settings subsystem (no game instance). */
	bool Bind(UObject* Owner, FCallback Callback);
	/** Subscribes to a specific subsystem (for owners without a world, such as local-player subsystems, and tests). */
	bool Bind(UGothamSettingsSubsystem* InSettings, UObject* Owner, FCallback Callback);
	/** Unsubscribes. Safe to call repeatedly, and after the subsystem is gone. */
	void Reset();

	bool IsBound() const { return Handle.IsValid() && Settings.IsValid(); }

private:
	TWeakObjectPtr<UGothamSettingsSubsystem> Settings;
	FDelegateHandle Handle;
};
