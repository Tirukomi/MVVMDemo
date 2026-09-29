// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/UnrealType.h"

namespace GothamUI
{
	/**
	 * Stops a widget ticking. C++-only widgets have no Blueprint class, so UUserWidget's default "Auto" frequency
	 * treats them as needing a native tick every frame, even though ours are entirely event-driven (view-model
	 * delegates). The property is private, so it is set through reflection, then the tick state is recomputed.
	 *
	 * Only use this on widgets with no widget animations and no latent actions: Never also stops those.
	 */
	inline void DisableTick(UUserWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
#if !UE_BUILD_SHIPPING
		// A/B switch for the performance harness: -GothamKeepTick restores the engine default (every widget ticks).
		static const bool bKeepTick = FParse::Param(FCommandLine::Get(), TEXT("GothamKeepTick"));
		if (bKeepTick)
		{
			return;
		}
#endif
		if (Widget->GetDesiredTickFrequency() != EWidgetTickFrequency::Never)
		{
			FProperty* Property = UUserWidget::StaticClass()->FindPropertyByName(TEXT("TickFrequency"));
			void* Value = Property ? Property->ContainerPtrToValuePtr<void>(Widget) : nullptr;
			if (!Value)
			{
				return;
			}
			if (FEnumProperty* Enum = CastField<FEnumProperty>(Property))
			{
				Enum->GetUnderlyingProperty()->SetIntPropertyValue(Value, static_cast<int64>(EWidgetTickFrequency::Never));
			}
			else if (FNumericProperty* Numeric = CastField<FNumericProperty>(Property))
			{
				Numeric->SetIntPropertyValue(Value, static_cast<int64>(EWidgetTickFrequency::Never));
			}
		}
		// Always bring the Slate side in line, even if the flag was already Never (pooled widgets are reconstructed
		// with a new SObjectWidget). UpdateCanTick does nothing without a world or before the SObjectWidget is
		// registered, and an SObjectWidget left ticking while the flag says Never trips UUserWidget::NativeTick's
		// ensure ("mismatching tick states"), so also switch the cached Slate widget off directly.
		Widget->UpdateCanTick();
		if (const TSharedPtr<SWidget> Slate = Widget->GetCachedWidget())
		{
			Slate->SetCanTick(false);
		}
	}
}
