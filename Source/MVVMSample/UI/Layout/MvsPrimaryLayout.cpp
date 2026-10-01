// Copyright IG. All Rights Reserved.

#include "UI/Layout/MvsPrimaryLayout.h"

#include "Accessibility/MvsSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Accessibility/MvsSettingsSubsystem.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Style/MvsMotion.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

UMvsScreenStack::UMvsScreenStack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransitionType = ECommonSwitcherTransition::FadeOnly;
	TransitionCurveType = ETransitionCurve::QuadOut;
	TransitionDuration = MvsMotion::ScreenSeconds;
}

TSharedRef<SWidget> UMvsPrimaryLayout::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		Layers.Reset();
		LayerBoxes.Reset();
		for (int32 i = 0; i < static_cast<int32>(EMvsUILayer::Count); ++i)
		{
			UOverlay* Box = WidgetTree->ConstructWidget<UOverlay>();
			Box->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UOverlaySlot* BoxSlot = Root->AddChildToOverlay(Box);
			BoxSlot->SetHorizontalAlignment(HAlign_Fill);
			BoxSlot->SetVerticalAlignment(VAlign_Fill);
			LayerBoxes.Add(Box);

			UCommonActivatableWidgetStack* Stack = WidgetTree->ConstructWidget<UMvsScreenStack>();
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

void UMvsPrimaryLayout::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData& Data) { ApplySettings(Data); });
	if (const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this))
	{
		ApplySettings(Settings->GetSettings());
	}
}

void UMvsPrimaryLayout::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UMvsPrimaryLayout::ApplySettings(const FMvsSettingsData& Data)
{
	UIScale = Data.GetUIScale();
	const float Seconds = MvsMotion::IsReduced(this) ? 0.f : MvsMotion::ScreenSeconds;
	for (UCommonActivatableWidgetStack* Stack : Layers)
	{
		if (Stack && !FMath::IsNearlyEqual(Stack->GetTransitionDuration(), Seconds))
		{
			Stack->SetTransitionDuration(Seconds);
		}
	}
}

UCommonActivatableWidgetStack* UMvsPrimaryLayout::GetLayer(EMvsUILayer Layer) const
{
	return Layers.IsValidIndex(static_cast<int32>(Layer)) ? Layers[static_cast<int32>(Layer)].Get() : nullptr;
}

void UMvsPrimaryLayout::SetLayerInteractive(EMvsUILayer Layer, bool bInteractive)
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

bool UMvsPrimaryLayout::IsLayerInteractive(EMvsUILayer Layer) const
{
	const UOverlay* Box = LayerBoxes.IsValidIndex(static_cast<int32>(Layer)) ? LayerBoxes[static_cast<int32>(Layer)].Get() : nullptr;
	return Box && Box->GetVisibility() != ESlateVisibility::HitTestInvisible;
}
