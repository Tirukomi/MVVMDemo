// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Layout/GothamPrimaryLayout.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "Widgets/Layout/SDPIScaler.h"
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
		LayerBoxes.Reset();
		for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Count); ++i)
		{
			UOverlay* Box = WidgetTree->ConstructWidget<UOverlay>();
			Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box);
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			BoxSlot->SetVerticalAlignment(VAlign_Fill);
			LayerBoxes.Add(Box);

			UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UGothamScreenStack>();
			UOverlaySlot* StackSlot = Box->AddChildToOverlay(Stack);
			StackSlot->SetHorizontalAlignment(HAlign_Fill);
			StackSlot->SetVerticalAlignment(VAlign_Fill);
			Layers.Add(Stack);
		}
	}
	// The scale is the layout's, not a global: it ends with this widget (no engine setting outlives a PIE session).
	return SNew(SDPIScaler)
		.DPIScale(TAttribute<float>::CreateWeakLambda(this, [this]() { return UIScale; }))
		[
			Super::RebuildWidget()
		];
}

void UGothamPrimaryLayout::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData& Data) { ApplySettings(Data); });
	if (const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		ApplySettings(Settings->GetSettings());
	}
}

void UGothamPrimaryLayout::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamPrimaryLayout::ApplySettings(const FGothamSettingsData& Data)
{
	UIScale = Data.GetUIScale();
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

void UGothamPrimaryLayout::SetLayerInteractive(EGothamUILayer Layer, bool bInteractive)
{
	if (UOverlay* Box = LayerBoxes.IsValidIndex(static_cast<int32>(Layer)) ? LayerBoxes[static_cast<int32>(Layer)].Get() : nullptr)
	{
		const ESlateVisibility Wanted = bInteractive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::HitTestInvisible;
		if (Box->GetVisibility() != Wanted)
		{
			Box->SetVisibility(Wanted);
		}
	}
}

bool UGothamPrimaryLayout::IsLayerInteractive(EGothamUILayer Layer) const
{
	const UOverlay* Box = LayerBoxes.IsValidIndex(static_cast<int32>(Layer)) ? LayerBoxes[static_cast<int32>(Layer)].Get() : nullptr;
	return Box && Box->GetVisibility() != ESlateVisibility::HitTestInvisible;
}
