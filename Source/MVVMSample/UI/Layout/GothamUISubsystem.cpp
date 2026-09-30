// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Layout/GothamUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "GameFramework/PlayerController.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamPrimaryLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UGothamUISubsystem::Deinitialize()
{
	Layout = nullptr;
	Super::Deinitialize();
}

void UGothamUISubsystem::EnsureLayout(APlayerController* Owner)
{
	if (Layout || !Owner)
	{
		return;
	}

	Layout = CreateWidget<UGothamPrimaryLayout>(Owner, UGothamPrimaryLayout::StaticClass());
	Layout->AddToPlayerScreen();

	for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Count); ++i)
	{
		const EGothamUILayer LayerId = static_cast<EGothamUILayer>(i);
		Layout->GetLayer(LayerId)->OnDisplayedWidgetChanged().AddWeakLambda(this,
			[this, LayerId](UCommonActivatableWidget*) { HandleLayerChanged(LayerId); });
		Layout->GetLayer(LayerId)->OnTransitioningChanged.AddWeakLambda(this, [this, i](UCommonActivatableWidgetContainerBase*, bool bTransitioning)
		{
			TransitioningLayers = bTransitioning ? (TransitioningLayers | (1u << i)) : (TransitioningLayers & ~(1u << i));
		});
	}
}

UCommonActivatableWidget* UGothamUISubsystem::PushScreen(EGothamUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	if (!Layout || !ScreenClass)
	{
		return nullptr;
	}
	return Layout->GetLayer(Layer)->AddWidget<UCommonActivatableWidget>(ScreenClass);
}

bool UGothamUISubsystem::PopTopScreen()
{
	const EGothamUILayer Layer = Tracker.GetTopDismissableLayer();
	if (Layer == EGothamUILayer::Count || !Layout)
	{
		return false;
	}
	if (UCommonActivatableWidget* Top = Layout->GetLayer(Layer)->GetActiveWidget())
	{
		Top->DeactivateWidget();
		return true;
	}
	return false;
}

bool UGothamUISubsystem::IsCovered(const UCommonActivatableWidget* Screen) const
{
	for (int32 i = 0; Layout && Screen && i < static_cast<int32>(EGothamUILayer::Count); ++i)
	{
		if (const UCommonActivatableWidgetContainerBase* Stack = Layout->GetLayer(static_cast<EGothamUILayer>(i)))
		{
			// A push adds the new screen to the list before the one below is deactivated.
			const TArray<UCommonActivatableWidget*>& List = Stack->GetWidgetList();
			const int32 Index = List.Find(const_cast<UCommonActivatableWidget*>(Screen));
			if (Index != INDEX_NONE)
			{
				return Index < List.Num() - 1;
			}
		}
	}
	return false;
}

void UGothamUISubsystem::TogglePauseMenu()
{
	if (!PopTopScreen())
	{
		PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->PauseMenuClass.LoadSynchronous());
	}
}

void UGothamUISubsystem::SetLayoutVisible(bool bVisible)
{
	if (Layout)
	{
		Layout->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGothamUISubsystem::ToggleClueLog()
{
	if (!Layout)
	{
		return;
	}
	const TSubclassOf<UCommonActivatableWidget> ClueLogClass = GetDefault<UGothamUISettings>()->ClueLogClass.LoadSynchronous();
	if (UCommonActivatableWidget* Wheel = Tracker.IsLayerOccupied(EGothamUILayer::GameMenu) ? Layout->GetLayer(EGothamUILayer::GameMenu)->GetActiveWidget() : nullptr)
	{
		Wheel->DeactivateWidget();
	}
	UCommonActivatableWidget* Top = Layout->GetLayer(EGothamUILayer::Menu)->GetActiveWidget();
	if (Top && ClueLogClass && Top->IsA(ClueLogClass))
	{
		Top->DeactivateWidget();
		return;
	}
	PushScreen(EGothamUILayer::Menu, ClueLogClass);
}

void UGothamUISubsystem::OpenGadgetWheel()
{
	if (!Tracker.IsMenuOpen() && !Tracker.IsLayerOccupied(EGothamUILayer::GameMenu))
	{
		PushScreen(EGothamUILayer::GameMenu, GetDefault<UGothamUISettings>()->GadgetWheelClass.LoadSynchronous());
	}
}

void UGothamUISubsystem::HandleLayerChanged(EGothamUILayer Layer)
{
	const EGothamInputContext Before = Tracker.GetInputContext();
	Tracker.SetLayerOccupied(Layer, Layout->GetLayer(Layer)->GetActiveWidget() != nullptr);

	const EGothamInputContext After = Tracker.GetInputContext();
	if (After != Before)
	{
		OnInputContextChanged.Broadcast(After);
	}
}
