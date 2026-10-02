// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FClassProperty;
class FObjectProperty;
class FProperty;

/**
 * The engine properties the game writes through reflection because the engine offers no setter (the documented
 * exceptions in Docs/CodingStandard.md). Each lookup is made once; if an engine upgrade renames or removes the
 * property, it raises an ensure naming it (the gate's log scan fails on ensures) and returns null, and the caller
 * skips the write. Mvs.Engine.Properties checks all of them, so an upgrade fails a test before it fails silently.
 */
namespace MvsEngineProperties
{
	/** UUserWidget::TickFrequency (private, no setter): MvsUI::DisableTick. */
	MVVMSAMPLE_API FProperty* WidgetTickFrequency();
	/** UCommonBoundActionBar::ActionButtonClass (a designer setting, no setter): UMvsActionBar's prompt class. */
	MVVMSAMPLE_API FClassProperty* ActionBarButtonClass();
	/** UInputAction::PlayerMappableKeySettings (protected, normally authored on the asset): code-built rebindable actions. */
	MVVMSAMPLE_API FObjectProperty* InputActionKeySettings();
}
