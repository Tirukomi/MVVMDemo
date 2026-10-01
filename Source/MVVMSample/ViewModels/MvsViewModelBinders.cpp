// Copyright IG. All Rights Reserved.

#include "ViewModels/MvsViewModelBinders.h"

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Core/MvsCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "CommonInputSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/ClueDataAsset.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/ForensicComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "Input/MvsBindings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/ForensicViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/ThreatViewModel.h"

// --- Vitals ---------------------------------------------------------------------------------------------------------

void UMvsVitalsBinder::Initialize(ULocalPlayer& Player)
{
	Vitals = AddViewModel<UPlayerVitalsViewModel>();
}

void UMvsVitalsBinder::Bind(AMvsCharacter& Character)
{
	UHealthComponent* Health = Character.GetHealthComponent();
	Subscriptions.Add(Health, &UHealthComponent::OnHealthChanged, this, [this](float Current, float Max) { Vitals->SetVitals(Current, Max); });
	if (Health)
	{
		Health->BroadcastCurrent();
	}
}

// --- Gadgets --------------------------------------------------------------------------------------------------------

void UMvsGadgetBinder::Initialize(ULocalPlayer& InPlayer)
{
	Player = &InPlayer;
	GadgetBar = AddViewModel<UGadgetBarViewModel>();
}

void UMvsGadgetBinder::Bind(AMvsCharacter& Character)
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
	const TWeakObjectPtr<AMvsCharacter> WeakCharacter(&Character);
	Subscriptions.Add(GadgetBar.Get(), &UGadgetBarViewModel::OnUseRequested, this, [WeakCharacter](int32 Slot)
	{
		if (AMvsCharacter* Hero = WeakCharacter.Get())
		{
			Hero->UseGadget(Slot);
		}
	});

	// Static slot data (name, key hint, tint) is set once per bind; cooldowns stream in afterwards.
	const TArray<FMvsGadgetDefinition>& Defs = Gadgets->GetGadgets();
	GadgetBar->SetSlotCount(Defs.Num());
	for (int32 i = 0; i < Defs.Num(); ++i)
	{
		GadgetBar->GetSlot(i)->SetDefinition(Defs[i].DisplayName, FText::AsNumber(i + 1), Defs[i].Tint, Defs[i].IconIndex);
	}
	// Key hints follow the player's bindings and the device in use, and change when either does.
	const ULocalPlayer* LocalPlayer = Player.Get();
	Subscriptions.Add(UCommonInputSubsystem::Get(LocalPlayer), &UCommonInputSubsystem::OnInputMethodChangedNative, this,
		[this](ECommonInputType) { RefreshHotkeys(); });
	if (const auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		if (UEnhancedInputUserSettings* InputSettings = Input->GetUserSettings())
		{
			InputSettings->OnSettingsChanged.AddUniqueDynamic(this, &UMvsGadgetBinder::HandleInputSettingsChanged);
			BoundInputSettings = InputSettings;
		}
	}
	RefreshHotkeys();
	Gadgets->BroadcastAll();
}

void UMvsGadgetBinder::Unbind()
{
	if (UEnhancedInputUserSettings* InputSettings = BoundInputSettings.Get())
	{
		InputSettings->OnSettingsChanged.RemoveDynamic(this, &UMvsGadgetBinder::HandleInputSettingsChanged);
	}
	BoundInputSettings.Reset();
	Super::Unbind();
}

void UMvsGadgetBinder::RefreshHotkeys()
{
	const UEnhancedInputUserSettings* InputSettings = BoundInputSettings.Get();
	const UEnhancedPlayerMappableKeyProfile* Profile = InputSettings ? InputSettings->GetActiveKeyProfile() : nullptr;
	// The device in use decides which of an action's keys the hint shows (the keyboard key, or the gamepad button in
	// the connected pad's naming), as the menu prompts do.
	const UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(Player.Get());
	const bool bGamepad = Input && Input->GetCurrentInputType() == ECommonInputType::Gamepad;
	const EMvsGamepadStyle PadStyle = Input ? MvsBindings::GamepadStyleFromName(Input->GetCurrentGamepadName()) : EMvsGamepadStyle::Xbox;
	for (int32 i = 0; i < GadgetBar->GetSlots().Num(); ++i)
	{
		FKey Key;
		if (const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(*FString::Printf(TEXT("Gadget%d"), i + 1)) : nullptr)
		{
			for (const FPlayerKeyMapping& Mapping : Row->Mappings)
			{
				const FKey Current = Mapping.GetCurrentKey();
				if (Current.IsValid() && Current.IsGamepadKey() == bGamepad)
				{
					Key = Current;
					break;
				}
			}
		}
		GadgetBar->GetSlot(i)->SetHotkey(Key.IsValid() ? MvsBindings::GetKeyLabel(Key, PadStyle) : FText::AsNumber(i + 1));
	}
}

// --- Combo ----------------------------------------------------------------------------------------------------------

void UMvsComboBinder::Initialize(ULocalPlayer& Player)
{
	Combo = AddViewModel<UComboViewModel>();
}

void UMvsComboBinder::Bind(AMvsCharacter& Character)
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

// --- Forensic ------------------------------------------------------------------------------------------------------

void UMvsForensicBinder::Initialize(ULocalPlayer& Player)
{
	Forensic = AddViewModel<UForensicViewModel>();
}

void UMvsForensicBinder::Bind(AMvsCharacter& Character)
{
	UForensicComponent* ForensicComp = Character.GetForensicComponent();
	Subscriptions.Add(ForensicComp, &UForensicComponent::OnForensicChanged, this, [this](bool bActive, float Alpha) { Forensic->SetState(bActive, Alpha); });
	Subscriptions.Add(ForensicComp, &UForensicComponent::OnAnalysisChanged, this, [this](FName ClueId, float Progress) { Forensic->SetAnalysis(ClueId, Progress); });
	if (ForensicComp)
	{
		ForensicComp->BroadcastCurrent();
	}
}

// --- Threats --------------------------------------------------------------------------------------------------------

void UMvsThreatBinder::Initialize(ULocalPlayer& Player)
{
	Threats = AddViewModel<UThreatViewModel>();
}

void UMvsThreatBinder::Bind(AMvsCharacter& Character)
{
	// One snapshot list per frame from the world's threat subsystem.
	UMvsThreatSubsystem* ThreatSub = Character.GetWorld() ? Character.GetWorld()->GetSubsystem<UMvsThreatSubsystem>() : nullptr;
	Subscriptions.Add(ThreatSub, &UMvsThreatSubsystem::OnThreatsUpdated, this, [this](const TArray<FMvsThreatSnapshot>& Snapshots)
	{
		Threats->SetThreats(Snapshots);
	});
}

// --- Clues ----------------------------------------------------------------------------------------------------------

void UMvsClueBinder::Initialize(ULocalPlayer& Player)
{
	Clues = AddViewModel<UClueListViewModel>();
	Objectives = AddViewModel<UObjectivesViewModel>();
	Subtitles = AddViewModel<USubtitleViewModel>();
	Objectives->SetProgress(0, 0);

	// The objective follows the list, whoever changes it (the level's clues, or a dev aid adding or removing fakes).
	MvsMVVM::Bind(Clues, MvsMVVM::FDelegate::CreateWeakLambda(this, [this](UObject*, MvsMVVM::FFieldId) { RefreshObjectives(); }),
		{ UClueListViewModel::FFieldNotificationClassDescriptor::Entries });

	// Subtitle size and backing panel follow accessibility settings. A local player has no world, so the settings
	// subsystem is looked up on the game instance.
	UGameInstance* GameInstance = Player.GetGameInstance();
	if (UMvsSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<UMvsSettingsSubsystem>() : nullptr)
	{
		auto Apply = [this](const FMvsSettingsData& Data) { Subtitles->SetPresentation(Data.GetSubtitleFontSize(), Data.bSubtitleBackground); };
		SettingsListener.Bind(Settings, this, Apply);
		Apply(Settings->GetSettings());
	}
}

void UMvsClueBinder::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SettingsListener.Reset();
	Super::Deinitialize();
}

void UMvsClueBinder::Bind(AMvsCharacter& Character)
{
	World = Character.GetWorld();
	UForensicComponent* ForensicComp = Character.GetForensicComponent();
	Subscriptions.Add(ForensicComp, &UForensicComponent::OnClueScanned, this, [this](const UClueDataAsset* Clue) { HandleClueScanned(Clue); });
	const TWeakObjectPtr<AMvsCharacter> WeakCharacter(&Character);
	Subscriptions.Add(ForensicComp, &UForensicComponent::OnCluesCollected, this, [this, WeakCharacter]() { Rebuild(WeakCharacter.Get()); });
	Rebuild(&Character);
}

/** One entry per clue placed in the level; scanning flips them to discovered. */
void UMvsClueBinder::Rebuild(const AMvsCharacter* Character)
{
	const UForensicComponent* ForensicComp = Character ? Character->GetForensicComponent() : nullptr;
	if (!ForensicComp)
	{
		return;
	}
	TArray<TObjectPtr<UClueEntryViewModel>> Entries;
	for (const UClueDataAsset* Clue : ForensicComp->GetAllClues())
	{
		UClueEntryViewModel* Entry = NewObject<UClueEntryViewModel>(this);
		Entry->Initialize(Clue->ClueId, Clue->Title, Clue->Description, Clue->Thumbnail);
		Entry->SetDiscovered(ForensicComp->IsScanned(Clue->ClueId));
		FVector Location;
		if (ForensicComp->GetClueLocation(Clue->ClueId, Location))
		{
			Entry->SetWorldLocation(Location);
		}
		Entries.Add(Entry);
	}
	Clues->SetEntries(MoveTemp(Entries));
	RefreshObjectives();
}

void UMvsClueBinder::HandleClueScanned(const UClueDataAsset* Clue)
{
	if (Clue)
	{
		Clues->MarkDiscovered(Clue->ClueId);
		RefreshObjectives();
		ShowSubtitle(NSLOCTEXT("Mvs.Subtitles", "CaseFile", "Case file"),
			FText::Format(NSLOCTEXT("Mvs.Subtitles", "ClueFound", "{0}. {1}"), Clue->Title, Clue->Description), 5.f);
	}
}

void UMvsClueBinder::RefreshObjectives()
{
	int32 RealClues = 0;
	for (const UClueEntryViewModel* Entry : Clues->GetEntries())
	{
		RealClues += (Entry && !Entry->IsDebug()) ? 1 : 0;
	}
	// Debug clues are never discovered, so only the total needs filtering.
	Objectives->SetProgress(Clues->GetDiscoveredCount(), RealClues);
}

void UMvsClueBinder::ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds)
{
	// Real time, so a hit-stop does not stretch it, but not while paused: a line that appears just before the player
	// pauses is still there afterwards (second review 3).
	Subtitles->ShowFor(Speaker, Line, Seconds);
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SubtitleHideHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float DeltaSeconds)
	{
		const UWorld* PlayerWorld = World.Get();
		return Subtitles->Advance(PlayerWorld && PlayerWorld->IsPaused() ? 0.f : DeltaSeconds);
	}));
}

void UMvsClueBinder::AddDebugClues(int32 Count)
{
	TArray<TObjectPtr<UClueEntryViewModel>> Entries = Clues->GetEntries();
	for (int32 i = 0; i < Count; ++i)
	{
		UClueEntryViewModel* Entry = NewObject<UClueEntryViewModel>(this);
		Entry->Initialize(*FString::Printf(TEXT("Debug_%d"), i),
			FText::Format(NSLOCTEXT("Mvs.Clues", "DebugTitle", "Debug clue {0}"), FText::AsNumber(i + 1)),
			NSLOCTEXT("Mvs.Clues", "DebugBody", "Generated to stress the virtualised list."), TSoftObjectPtr<UTexture2D>());
		Entry->SetIsDebug(true);
		Entries.Add(Entry);
	}
	Clues->SetEntries(MoveTemp(Entries));
}
