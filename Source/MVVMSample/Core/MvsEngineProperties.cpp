// Copyright IG. All Rights Reserved.

#include "Core/MvsEngineProperties.h"

#include "Blueprint/UserWidget.h"
#include "Input/CommonBoundActionBar.h"
#include "InputAction.h"
#include "UObject/UnrealType.h"

namespace MvsEngineProperties
{
	namespace
	{
		template<typename TProperty>
		TProperty* Find(UClass* Class, const TCHAR* Name)
		{
			TProperty* Property = FindFProperty<TProperty>(Class, Name);
			// Second review 16: these used to skip the write without a word when the property was gone.
			ensureMsgf(Property, TEXT("%s::%s is gone or changed type (engine upgrade?); the game can no longer set it. See Docs/CodingStandard.md, reflection exceptions."),
				*Class->GetName(), Name);
			return Property;
		}
	}

	FProperty* WidgetTickFrequency()
	{
		static FProperty* const Property = Find<FProperty>(UUserWidget::StaticClass(), TEXT("TickFrequency"));
		return Property;
	}

	FClassProperty* ActionBarButtonClass()
	{
		static FClassProperty* const Property = Find<FClassProperty>(UCommonBoundActionBar::StaticClass(), TEXT("ActionButtonClass"));
		return Property;
	}

	FObjectProperty* InputActionKeySettings()
	{
		static FObjectProperty* const Property = Find<FObjectProperty>(UInputAction::StaticClass(), TEXT("PlayerMappableKeySettings"));
		return Property;
	}
}
