// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Layout/GothamPrimaryLayout.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Style/GothamMotion.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UGothamScreenStack::UGothamScreenStack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransitionType = ECommonSwitcherTransition::FadeOnly;
	TransitionCurveType = ETransitionCurve::QuadOut;
	TransitionDuration = GothamMotion::ScreenSeconds;
}

TSharedRef<SWidget> UGothamPrimaryLayout::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Layers.Reset();
		for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Count); ++i)
		{
			UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UGothamScreenStack>();
			UOverlaySlot* StackSlot = Root->AddChildToOverlay(Stack);
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Fill);
			Layers.Add(Stack);
		}
	}
	return Super::RebuildWidget();
}

void UGothamPrimaryLayout::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyMotionSetting(); });
	ApplyMotionSetting();
}

void UGothamPrimaryLayout::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamPrimaryLayout::ApplyMotionSetting()
{
	const float Seconds = GothamMotion::IsReduced(this) ? 0.f : GothamMotion::ScreenSeconds;
	for (UCommonActivatableWidgetStack* Stack : Layers)
	{
		if (Stack && !FMath::IsNearlyEqual(Stack->GetTransitionDuration(), Seconds))
		{
			Stack->SetTransitionDuration(Seconds);
		}
	}
}

UCommonActivatableWidgetStack* UGothamPrimaryLayout::GetLayer(EGothamUILayer Layer) const
{
	return Layers.IsValidIndex(static_cast<int32>(Layer)) ? Layers[static_cast<int32>(Layer)].Get() : nullptr;
}
