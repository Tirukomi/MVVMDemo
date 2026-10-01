// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Layout/GothamUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamPrimaryLayout.h"
#include "UI/Screens/GothamScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UGothamUISubsystem::Deinitialize()
{
	if (ScreenClasses.IsValid())
	{
		ScreenClasses->ReleaseHandle();
		ScreenClasses.Reset();
	}
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

	const TArray<FSoftObjectPath> Paths = GetDefault<UGothamUISettings>()->GetPreloadPaths();
	if (!Paths.IsEmpty())
	{
		ScreenClasses = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}

	for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Count); ++i)
	{
		const EGothamUILayer LayerId = static_cast<EGothamUILayer>(i);
		UCommonActivatableWidgetStack* Stack = Layout->GetLayer(LayerId);
		if (!ensureMsgf(Stack, TEXT("The primary layout has no stack for UI layer %d"), i))
		{
			continue;
		}
		Stack->OnDisplayedWidgetChanged().AddWeakLambda(this,
			[this, LayerId](UCommonActivatableWidget*) { HandleLayerChanged(LayerId); });
		Stack->OnTransitioningChanged.AddWeakLambda(this, [this, i](UCommonActivatableWidgetContainerBase*, bool bTransitioning)
		{
			TransitioningLayers = bTransitioning ? (TransitioningLayers | (1u << i)) : (TransitioningLayers & ~(1u << i));
		});
	}
}

UCommonActivatableWidget* UGothamUISubsystem::PushScreen(EGothamUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	UCommonActivatableWidgetStack* Stack = Layout && ScreenClass ? Layout->GetLayer(Layer) : nullptr;
	return Stack ? Stack->AddWidget<UCommonActivatableWidget>(ScreenClass) : nullptr;
}

UCommonActivatableWidget* UGothamUISubsystem::PushScreen(EGothamUILayer Layer, const TSoftClassPtr<UCommonActivatableWidget>& ScreenClass)
{
	return PushScreen(Layer, UGothamUISettings::Resolve(ScreenClass));
}

UCommonActivatableWidget* UGothamUISubsystem::GetActiveScreen(EGothamUILayer Layer) const
{
	const UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayer(Layer) : nullptr;
	return Stack ? Stack->GetActiveWidget() : nullptr;
}

bool UGothamUISubsystem::AreScreensLoaded() const
{
	return !ScreenClasses.IsValid() || ScreenClasses->HasLoadCompleted();
}

bool UGothamUISubsystem::PopTopScreen()
{
	const EGothamUILayer Layer = Tracker.GetTopDismissableLayer();
	if (UCommonActivatableWidget* Top = GetActiveScreen(Layer))
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
		PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->PauseMenuClass);
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
	const TSubclassOf<UCommonActivatableWidget> ClueLogClass = UGothamUISettings::Resolve(GetDefault<UGothamUISettings>()->ClueLogClass);
	if (UCommonActivatableWidget* Wheel = GetActiveScreen(EGothamUILayer::GameMenu))
	{
		Wheel->DeactivateWidget();
	}
	UCommonActivatableWidget* Top = GetActiveScreen(EGothamUILayer::Menu);
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
		PushScreen(EGothamUILayer::GameMenu, GetDefault<UGothamUISettings>()->GadgetWheelClass);
	}
}

void UGothamUISubsystem::HandleLayerChanged(EGothamUILayer Layer)
{
	Tracker.SetLayerOccupied(Layer, GetActiveScreen(Layer) != nullptr);
	UpdateBackdrops();
	// A confirmation must be answered first: the screens behind it stay visible but cannot be clicked or navigated to.
	const bool bModalOpen = Tracker.IsLayerOccupied(EGothamUILayer::Modal);
	for (int32 i = 0; i < static_cast<int32>(EGothamUILayer::Modal); ++i)
	{
		Layout->SetLayerInteractive(static_cast<EGothamUILayer>(i), !bModalOpen);
	}
}

void UGothamUISubsystem::UpdateBackdrops()
{
	bool bBlurAbove = false;
	for (int32 i = static_cast<int32>(EGothamUILayer::Count) - 1; i >= 0; --i)
	{
		if (UGothamScreen* Screen = Cast<UGothamScreen>(GetActiveScreen(static_cast<EGothamUILayer>(i))))
		{
			if (Screen->HasBackdropBlur())
			{
				Screen->SetBackdropBlurEnabled(!bBlurAbove);
				bBlurAbove = true;
			}
		}
	}
}
