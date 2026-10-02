// Copyright IG. All Rights Reserved.

#include "UI/Layout/MvsUISubsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonActivatableWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/PlayerController.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsPrimaryLayout.h"
#include "UI/Screens/MvsScreen.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UMvsUISubsystem::Deinitialize()
{
	if (ScreenClasses.IsValid())
	{
		ScreenClasses->ReleaseHandle();
		ScreenClasses.Reset();
	}
	Layout = nullptr;
	Super::Deinitialize();
}

void UMvsUISubsystem::EnsureLayout(APlayerController* Owner)
{
	if (Layout || !Owner)
	{
		return;
	}

	Layout = CreateWidget<UMvsPrimaryLayout>(Owner, UMvsPrimaryLayout::StaticClass());
	Layout->AddToPlayerScreen();

	const TArray<FSoftObjectPath> Paths = GetDefault<UMvsUISettings>()->GetPreloadPaths();
	if (!Paths.IsEmpty())
	{
		ScreenClasses = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate(), FStreamableManager::AsyncLoadHighPriority);
	}

	for (int32 i = 0; i < static_cast<int32>(EMvsUILayer::Count); ++i)
	{
		const EMvsUILayer LayerId = static_cast<EMvsUILayer>(i);
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

UCommonActivatableWidget* UMvsUISubsystem::PushScreen(EMvsUILayer Layer, TSubclassOf<UCommonActivatableWidget> ScreenClass)
{
	UCommonActivatableWidgetStack* Stack = Layout && ScreenClass ? Layout->GetLayer(Layer) : nullptr;
	return Stack ? Stack->AddWidget<UCommonActivatableWidget>(ScreenClass) : nullptr;
}

UCommonActivatableWidget* UMvsUISubsystem::PushScreen(EMvsUILayer Layer, const TSoftClassPtr<UCommonActivatableWidget>& ScreenClass)
{
	return PushScreen(Layer, UMvsUISettings::Resolve(ScreenClass));
}

UCommonActivatableWidget* UMvsUISubsystem::GetActiveScreen(EMvsUILayer Layer) const
{
	const UCommonActivatableWidgetStack* Stack = Layout ? Layout->GetLayer(Layer) : nullptr;
	return Stack ? Stack->GetActiveWidget() : nullptr;
}

bool UMvsUISubsystem::AreScreensLoaded() const
{
	return !ScreenClasses.IsValid() || ScreenClasses->HasLoadCompleted();
}

bool UMvsUISubsystem::PopTopScreen()
{
	const EMvsUILayer Layer = Tracker.GetTopDismissableLayer();
	if (UCommonActivatableWidget* Top = GetActiveScreen(Layer))
	{
		Top->DeactivateWidget();
		return true;
	}
	return false;
}

bool UMvsUISubsystem::IsCovered(const UCommonActivatableWidget* Screen) const
{
	for (int32 i = 0; Layout && Screen && i < static_cast<int32>(EMvsUILayer::Count); ++i)
	{
		if (const UCommonActivatableWidgetContainerBase* Stack = Layout->GetLayer(static_cast<EMvsUILayer>(i)))
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

bool UMvsUISubsystem::HandleShortcut(FName Action)
{
	const FMvsScreenShortcut* Shortcut = UMvsUISettings::FindShortcut(Action);
	if (!Shortcut || !Layout)
	{
		return false;
	}
	const UMvsUISettings* Settings = GetDefault<UMvsUISettings>();
	const TSubclassOf<UCommonActivatableWidget> ScreenClass = Settings->ResolveScreen(*Shortcut);
	switch (Shortcut->Kind)
	{
	case EMvsShortcutKind::OpenOrBack:
		if (!PopTopScreen())
		{
			PushScreen(Shortcut->Layer, ScreenClass);
		}
		break;
	case EMvsShortcutKind::Toggle:
		if (Shortcut->Layer != EMvsUILayer::GameMenu)
		{
			// An in-world overlay (the gadget wheel) gives way to a menu.
			if (UCommonActivatableWidget* Overlay = GetActiveScreen(EMvsUILayer::GameMenu))
			{
				Overlay->DeactivateWidget();
			}
		}
		if (UCommonActivatableWidget* Top = GetActiveScreen(Shortcut->Layer); Top && ScreenClass && Top->IsA(ScreenClass))
		{
			Top->DeactivateWidget();
		}
		else
		{
			PushScreen(Shortcut->Layer, ScreenClass);
		}
		break;
	case EMvsShortcutKind::Hold:
		if (!Tracker.IsMenuOpen() && !Tracker.IsLayerOccupied(Shortcut->Layer))
		{
			PushScreen(Shortcut->Layer, ScreenClass);
		}
		break;
	}
	return true;
}

void UMvsUISubsystem::SetLayoutVisible(bool bVisible)
{
	if (Layout)
	{
		Layout->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UMvsUISubsystem::HandleLayerChanged(EMvsUILayer Layer)
{
	Tracker.SetLayerOccupied(Layer, GetActiveScreen(Layer) != nullptr);
	UpdateBackdrops();
	// A confirmation must be answered first: the screens behind it stay visible but cannot be clicked or navigated to.
	const bool bModalOpen = Tracker.IsLayerOccupied(EMvsUILayer::Modal);
	for (int32 i = 0; i < static_cast<int32>(EMvsUILayer::Modal); ++i)
	{
		Layout->SetLayerInteractive(static_cast<EMvsUILayer>(i), !bModalOpen);
	}
}

void UMvsUISubsystem::UpdateBackdrops()
{
	bool bBlurAbove = false;
	for (int32 i = static_cast<int32>(EMvsUILayer::Count) - 1; i >= 0; --i)
	{
		if (UMvsScreen* Screen = Cast<UMvsScreen>(GetActiveScreen(static_cast<EMvsUILayer>(i))))
		{
			if (Screen->HasBackdropBlur())
			{
				Screen->SetBackdropBlurEnabled(!bBlurAbove);
				bBlurAbove = true;
			}
		}
	}
}
