// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GothamViewModelBinders.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Core/GothamCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/ClueDataAsset.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "Input/GothamBindings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/ThreatViewModel.h"

// --- Vitals ---------------------------------------------------------------------------------------------------------

void UGothamVitalsBinder::Initialize(ULocalPlayer& Player)
{
	Vitals = AddViewModel<UPlayerVitalsViewModel>();
}

void UGothamVitalsBinder::Bind(AGothamCharacter& Character)
{
	UHealthComponent* Health = Character.GetHealthComponent();
	Subscriptions.Add(Health, &UHealthComponent::OnHealthChanged, this, [this](float Current, float Max) { Vitals->SetVitals(Current, Max); });
	if (Health)
	{
		Health->BroadcastCurrent();
	}
}

// --- Gadgets --------------------------------------------------------------------------------------------------------

void UGothamGadgetBinder::Initialize(ULocalPlayer& InPlayer)
{
	Player = &InPlayer;
	GadgetBar = AddViewModel<UGadgetBarViewModel>();
}

void UGothamGadgetBinder::Bind(AGothamCharacter& Character)
{
	UGadgetComponent* Gadgets = Character.GetGadgetComponent();
	if (!Gadgets)
	{
		return;
	}
	Subscriptions.Add(Gadgets, &UGadgetComponent::OnCooldownChanged, this, [this](int32 Slot, float Remaining, float Total)
	{
		if (UGadgetSlotViewModel* SlotVM = GadgetBar->GetSlot(Slot))
		{
			SlotVM->SetCooldown(Remaining, Total);
		}
	});
	Subscriptions.Add(Gadgets, &UGadgetComponent::OnGadgetUsed, this, [this](int32 Slot) { GadgetBar->SetSelectedIndex(Slot); });
	// The bar's command: a view (the wheel) asked to use a gadget.
	const TWeakObjectPtr<AGothamCharacter> WeakCharacter(&Character);
	Subscriptions.Add(GadgetBar.Get(), &UGadgetBarViewModel::OnUseRequested, this, [WeakCharacter](int32 Slot)
	{
		if (AGothamCharacter* Hero = WeakCharacter.Get())
		{
			Hero->UseGadget(Slot);
		}
	});

	// Static slot data (name, key hint, tint) is set once per bind; cooldowns stream in afterwards.
	const TArray<FGothamGadgetDefinition>& Defs = Gadgets->GetGadgets();
	GadgetBar->SetSlotCount(Defs.Num());
	for (int32 i = 0; i < Defs.Num(); ++i)
	{
		GadgetBar->GetSlot(i)->SetDefinition(Defs[i].DisplayName, FText::AsNumber(i + 1), Defs[i].Tint, Defs[i].IconIndex);
	}
	// Key hints follow the player's bindings, and change when they are rebound.
	const ULocalPlayer* LocalPlayer = Player.Get();
	if (const auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		if (UEnhancedInputUserSettings* InputSettings = Input->GetUserSettings())
		{
			InputSettings->OnSettingsChanged.AddUniqueDynamic(this, &UGothamGadgetBinder::HandleInputSettingsChanged);
			BoundInputSettings = InputSettings;
		}
	}
	RefreshHotkeys();
	Gadgets->BroadcastAll();
}

void UGothamGadgetBinder::Unbind()
{
	if (UEnhancedInputUserSettings* InputSettings = BoundInputSettings.Get())
	{
		InputSettings->OnSettingsChanged.RemoveDynamic(this, &UGothamGadgetBinder::HandleInputSettingsChanged);
	}
	BoundInputSettings.Reset();
	Super::Unbind();
}

void UGothamGadgetBinder::RefreshHotkeys()
{
	const UEnhancedInputUserSettings* InputSettings = BoundInputSettings.Get();
	const UEnhancedPlayerMappableKeyProfile* Profile = InputSettings ? InputSettings->GetActiveKeyProfile() : nullptr;
	for (int32 i = 0; i < GadgetBar->GetSlots().Num(); ++i)
	{
		FKey Key;
		if (const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(*FString::Printf(TEXT("Gadget%d"), i + 1)) : nullptr)
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				if (Mapping.GetSlot() == EPlayerMappableKeySlot::First)
				{
					Key = Mapping.GetCurrentKey();
				}
			}
		}
		GadgetBar->GetSlot(i)->SetHotkey(Key.IsValid() ? GothamBindings::GetKeyLabel(Key) : FText::AsNumber(i + 1));
	}
}

// --- Combo ----------------------------------------------------------------------------------------------------------

void UGothamComboBinder::Initialize(ULocalPlayer& Player)
{
	Combo = AddViewModel<UComboViewModel>();
}

void UGothamComboBinder::Bind(AGothamCharacter& Character)
{
	UComboComponent* ComboComp = Character.GetComboComponent();
	Subscriptions.Add(ComboComp, &UComboComponent::OnComboChanged, this, [this](int32 Hits, float Multiplier, float DecayAlpha)
	{
		Combo->SetCombo(Hits, Multiplier, DecayAlpha);
	});
	if (ComboComp)
	{
		ComboComp->BroadcastCurrent();
	}
}

// --- Detective ------------------------------------------------------------------------------------------------------

void UGothamDetectiveBinder::Initialize(ULocalPlayer& Player)
{
	Detective = AddViewModel<UDetectiveViewModel>();
}

void UGothamDetectiveBinder::Bind(AGothamCharacter& Character)
{
	UDetectiveComponent* DetectiveComp = Character.GetDetectiveComponent();
	Subscriptions.Add(DetectiveComp, &UDetectiveComponent::OnDetectiveChanged, this, [this](bool bActive, float Alpha) { Detective->SetState(bActive, Alpha); });
	Subscriptions.Add(DetectiveComp, &UDetectiveComponent::OnAnalysisChanged, this, [this](FName ClueId, float Progress) { Detective->SetAnalysis(ClueId, Progress); });
	if (DetectiveComp)
	{
		DetectiveComp->BroadcastCurrent();
	}
}

// --- Threats --------------------------------------------------------------------------------------------------------

void UGothamThreatBinder::Initialize(ULocalPlayer& Player)
{
	Threats = AddViewModel<UThreatViewModel>();
}

void UGothamThreatBinder::Bind(AGothamCharacter& Character)
{
	// One snapshot list per frame from the world's threat subsystem.
	UGothamThreatSubsystem* ThreatSub = Character.GetWorld() ? Character.GetWorld()->GetSubsystem<UGothamThreatSubsystem>() : nullptr;
	Subscriptions.Add(ThreatSub, &UGothamThreatSubsystem::OnThreatsUpdated, this, [this](const TArray<FGothamThreatSnapshot>& Snapshots)
	{
		Threats->SetThreats(Snapshots);
	});
}

// --- Clues ----------------------------------------------------------------------------------------------------------

void UGothamClueBinder::Initialize(ULocalPlayer& Player)
{
	Clues = AddViewModel<UClueListViewModel>();
	Objectives = AddViewModel<UObjectivesViewModel>();
	Subtitles = AddViewModel<USubtitleViewModel>();
	Objectives->SetProgress(0, 0);

	// The objective follows the list, whoever changes it (the level's clues, or a dev aid adding or removing fakes).
	GothamMVVM::Bind(Clues, GothamMVVM::FDelegate::CreateWeakLambda(this, [this](UObject*, GothamMVVM::FFieldId) { RefreshObjectives(); }),
		{ UClueListViewModel::FFieldNotificationClassDescriptor::Entries });

	// Subtitle size and backing panel follow accessibility settings. A local player has no world, so the settings
	// subsystem is looked up on the game instance.
	UGameInstance* GameInstance = Player.GetGameInstance();
	if (UGothamSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UGothamSettingsSubsystem>() : nullptr)
	{
		auto Apply = [this](const FGothamSettingsData& Data) { Subtitles->SetPresentation(Data.GetSubtitleFontSize(), Data.bSubtitleBackground); };
		SettingsListener.Bind(Settings, this, Apply);
		Apply(Settings->GetSettings());
	}
}

void UGothamClueBinder::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SettingsListener.Reset();
	Super::Deinitialize();
}

void UGothamClueBinder::Bind(AGothamCharacter& Character)
{
	UDetectiveComponent* DetectiveComp = Character.GetDetectiveComponent();
	Subscriptions.Add(DetectiveComp, &UDetectiveComponent::OnClueScanned, this, [this](const UClueDataAsset* Clue) { HandleClueScanned(Clue); });
	const TWeakObjectPtr<AGothamCharacter> WeakCharacter(&Character);
	Subscriptions.Add(DetectiveComp, &UDetectiveComponent::OnCluesCollected, this, [this, WeakCharacter]() { Rebuild(WeakCharacter.Get()); });
	Rebuild(&Character);
}

/** One entry per clue placed in the level; scanning flips them to discovered. */
void UGothamClueBinder::Rebuild(const AGothamCharacter* Character)
{
	const UDetectiveComponent* DetectiveComp = Character ? Character->GetDetectiveComponent() : nullptr;
	if (!DetectiveComp)
	{
		return;
	}
	TArray<TObjectPtr<UClueEntryViewModel>> Entries;
	for (const UClueDataAsset* Clue : DetectiveComp->GetAllClues())
	{
		UClueEntryViewModel* Entry = NewObject<UClueEntryViewModel>(this);
		Entry->Initialize(Clue->ClueId, Clue->Title, Clue->Description, Clue->Thumbnail);
		Entry->SetDiscovered(DetectiveComp->IsScanned(Clue->ClueId));
		FVector Location;
		if (DetectiveComp->GetClueLocation(Clue->ClueId, Location))
		{
			Entry->SetWorldLocation(Location);
		}
		Entries.Add(Entry);
	}
	Clues->SetEntries(MoveTemp(Entries));
	RefreshObjectives();
}

void UGothamClueBinder::HandleClueScanned(const UClueDataAsset* Clue)
{
	if (Clue)
	{
		Clues->MarkDiscovered(Clue->ClueId);
		RefreshObjectives();
		ShowSubtitle(NSLOCTEXT("Gotham.Subtitles", "Detective", "Detective"),
			FText::Format(NSLOCTEXT("Gotham.Subtitles", "ClueFound", "{0}. {1}"), Clue->Title, Clue->Description), 5.f);
	}
}

void UGothamClueBinder::RefreshObjectives()
{
	int32 RealClues = 0;
	for (const UClueEntryViewModel* Entry : Clues->GetEntries())
	{
		RealClues += (Entry && !Entry->IsDebug()) ? 1 : 0;
	}
	// Debug clues are never discovered, so only the total needs filtering.
	Objectives->SetProgress(Clues->GetDiscoveredCount(), RealClues);
}

void UGothamClueBinder::ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds)
{
	Subtitles->SetLine(Speaker, Line);
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SubtitleHideHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		Subtitles->Clear();
		return false;
	}), Seconds);
}

void UGothamClueBinder::AddDebugClues(int32 Count)
{
	TArray<TObjectPtr<UClueEntryViewModel>> Entries = Clues->GetEntries();
	for (int32 i = 0; i < Count; ++i)
	{
		UClueEntryViewModel* Entry = NewObject<UClueEntryViewModel>(this);
		Entry->Initialize(*FString::Printf(TEXT("Debug_%d"), i),
			FText::Format(NSLOCTEXT("Gotham.Clues", "DebugTitle", "Debug clue {0}"), FText::AsNumber(i + 1)),
			NSLOCTEXT("Gotham.Clues", "DebugBody", "Generated to stress the virtualised list."), TSoftObjectPtr<UTexture2D>());
		Entry->SetIsDebug(true);
		Entries.Add(Entry);
	}
	Clues->SetEntries(MoveTemp(Entries));
}
