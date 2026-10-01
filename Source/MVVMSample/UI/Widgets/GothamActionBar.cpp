// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamActionBar.h"

#include "CommonActivatableWidget.h"
#include "Input/UIActionBinding.h"
#include "UI/Widgets/GothamHintButton.h"

UGothamActionBar::UGothamActionBar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The prompt class is a designer setting with no setter; this bar is built in code, so it is set the way the
	// designer would set it, through the property.
	if (FClassProperty* ButtonClass = FindFProperty<FClassProperty>(UCommonBoundActionBar::StaticClass(), TEXT("ActionButtonClass")))
	{
		ButtonClass->SetObjectPropertyValue_InContainer(this, UGothamHintButton::StaticClass());
	}
	InitEntryBoxType(EDynamicBoxType::Horizontal);
	SetEntrySpacing(FVector2D(20.f, 0.f));
}

void UGothamActionBar::NativeOnActionButtonCreated(ICommonBoundActionButtonInterface* ActionButton, const FUIActionBindingHandle& RepresentedAction)
{
	Super::NativeOnActionButtonCreated(ActionButton, RepresentedAction);
	const UCommonActivatableWidget* Screen = GetTypedOuter<UCommonActivatableWidget>();
	const TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(RepresentedAction);
	const UWidget* Bound = Binding ? Binding->BoundWidget.Get() : nullptr;
	const bool bOwn = Bound && Screen && (Bound == Screen || Bound->IsIn(Screen));
	if (UUserWidget* Button = Cast<UUserWidget>(ActionButton))
	{
		Button->SetVisibility(bOwn ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
