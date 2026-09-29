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
	if (!PopTopScreen())
	{
		PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ClueLogClass.LoadSynchronous());
	}
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
