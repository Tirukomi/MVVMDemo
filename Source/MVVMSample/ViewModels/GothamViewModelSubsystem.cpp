// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GothamViewModelSubsystem.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Core/GothamCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Input/GothamBindings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Gameplay/ClueDataAsset.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "Engine/Texture2D.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/ThreatViewModel.h"
#include "Gameplay/ThreatSubsystem.h"
#include "Engine/World.h"

void UGothamViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Vitals = NewObject<UPlayerVitalsViewModel>(this);
	GadgetBar = NewObject<UGadgetBarViewModel>(this);
	Combo = NewObject<UComboViewModel>(this);
	Detective = NewObject<UDetectiveViewModel>(this);
	Objectives = NewObject<UObjectivesViewModel>(this);
	Clues = NewObject<UClueListViewModel>(this);
	// The objective follows the list, whoever changes it (the level's clues, or a dev aid adding or removing fakes).
	GothamMVVM::Bind(Clues, this, &UGothamViewModelSubsystem::OnCluesChanged, { UClueListViewModel::FFieldNotificationClassDescriptor::Entries });
	Subtitles = NewObject<USubtitleViewModel>(this);
	Threats = NewObject<UThreatViewModel>(this);
	Objectives->SetProgress(0, 0);

	// Subtitle size and backing panel follow accessibility settings.
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameInstance* GameInstance = LocalPlayer->GetGameInstance())
		{
			if (UGothamSettingsSubsystem* Settings = GameInstance->GetSubsystem<UGothamSettingsSubsystem>())
			{
				// A local-player subsystem has no world, so the settings subsystem is passed in directly.
				SettingsListener.Bind(Settings, this, [this](const FGothamSettingsData& Data) { HandleSettings(Data); });
				HandleSettings(Settings->GetSettings());
			}
		}
	}
}

void UGothamViewModelSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SettingsListener.Reset();
	Unbind();
	Super::Deinitialize();
}

UObject* UGothamViewModelSubsystem::FindViewModel(const UClass* ViewModelClass) const
{
	for (UObject* Candidate : { static_cast<UObject*>(Vitals), static_cast<UObject*>(GadgetBar), static_cast<UObject*>(Combo),
		static_cast<UObject*>(Detective), static_cast<UObject*>(Objectives), static_cast<UObject*>(Clues), static_cast<UObject*>(Subtitles),
		static_cast<UObject*>(Threats) })
	{
		if (Candidate && Candidate->GetClass()->IsChildOf(ViewModelClass))
		{
			return Candidate;
		}
	}
	return nullptr;
}

void UGothamViewModelSubsystem::Unbind()
{
	if (UHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.Remove(HealthHandle);
	}
	if (UGadgetComponent* Gadgets = BoundGadgets.Get())
	{
		Gadgets->OnCooldownChanged.Remove(GadgetHandle);
		Gadgets->OnGadgetUsed.Remove(GadgetUsedHandle);
	}
	if (UComboComponent* ComboComp = BoundCombo.Get())
	{
		ComboComp->OnComboChanged.Remove(ComboHandle);
	}
	BoundHealth.Reset();
	BoundGadgets.Reset();
	if (UDetectiveComponent* DetectiveComp = BoundDetective.Get())
	{
		DetectiveComp->OnDetectiveChanged.Remove(DetectiveHandle);
		DetectiveComp->OnClueScanned.Remove(ScanHandle);
		DetectiveComp->OnAnalysisChanged.Remove(AnalysisHandle);
		DetectiveComp->OnCluesCollected.Remove(CluesHandle);
	}
	if (UGothamThreatSubsystem* ThreatSub = BoundThreats.Get())
	{
		ThreatSub->OnThreatsUpdated.Remove(ThreatsHandle);
	}
	BoundThreats.Reset();
	BoundCombo.Reset();
	BoundDetective.Reset();
	if (UEnhancedInputUserSettings* InputSettings = BoundInputSettings.Get())
	{
		InputSettings->OnSettingsChanged.RemoveDynamic(this, &UGothamViewModelSubsystem::HandleInputSettingsChanged);
	}
	BoundInputSettings.Reset();
}

void UGothamViewModelSubsystem::HandleInputSettingsChanged(UEnhancedInputUserSettings* InputSettings)
{
	RefreshGadgetHotkeys();
}

void UGothamViewModelSubsystem::RefreshGadgetHotkeys()
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

void UGothamViewModelSubsystem::BindToCharacter(AGothamCharacter* Character)
{
	Unbind();
	if (!Character)
	{
		return;
	}

	BoundHealth = Character->GetHealthComponent();
	BoundGadgets = Character->GetGadgetComponent();
	BoundCombo = Character->GetComboComponent();
	BoundDetective = Character->GetDetectiveComponent();

	HealthHandle = BoundHealth->OnHealthChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleHealth);
	GadgetHandle = BoundGadgets->OnCooldownChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleGadgetCooldown);
	GadgetUsedHandle = BoundGadgets->OnGadgetUsed.AddWeakLambda(this, [this](int32 Slot) { GadgetBar->SetSelectedIndex(Slot); });
	ComboHandle = BoundCombo->OnComboChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleCombo);
	DetectiveHandle = BoundDetective->OnDetectiveChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleDetective);
	ScanHandle = BoundDetective->OnClueScanned.AddUObject(this, &UGothamViewModelSubsystem::HandleClueScanned);
	AnalysisHandle = BoundDetective->OnAnalysisChanged.AddWeakLambda(this, [this](FName ClueId, float Progress) { Detective->SetAnalysis(ClueId, Progress); });

	// Static slot data (name, key hint, tint) is set once per bind; cooldowns stream in afterwards.
	const TArray<FGothamGadgetDefinition>& Defs = BoundGadgets->GetGadgets();
	GadgetBar->SetSlotCount(Defs.Num());
	for (int32 i = 0; i < Defs.Num(); ++i)
	{
		GadgetBar->GetSlot(i)->SetDefinition(Defs[i].DisplayName, FText::AsNumber(i + 1), Defs[i].Tint, Defs[i].IconIndex);
	}
	// Key hints follow the player's bindings, and change when they are rebound.
	if (const auto* Input = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		if (UEnhancedInputUserSettings* InputSettings = Input->GetUserSettings())
		{
			InputSettings->OnSettingsChanged.AddUniqueDynamic(this, &UGothamViewModelSubsystem::HandleInputSettingsChanged);
			BoundInputSettings = InputSettings;
		}
	}
	RefreshGadgetHotkeys();

	// Hostiles live in the world, not on the character: the threat subsystem publishes one snapshot list per frame.
	if (UGothamThreatSubsystem* ThreatSub = Character->GetWorld()->GetSubsystem<UGothamThreatSubsystem>())
	{
		BoundThreats = ThreatSub;
		ThreatsHandle = ThreatSub->OnThreatsUpdated.AddWeakLambda(this, [this](const TArray<FGothamThreatSnapshot>& Snaps) { Threats->SetThreats(Snaps); });
	}

	CluesHandle = BoundDetective->OnCluesCollected.AddUObject(this, &UGothamViewModelSubsystem::RebuildClues);
	RebuildClues();

	BoundHealth->BroadcastCurrent();
	BoundGadgets->BroadcastAll();
	BoundCombo->BroadcastCurrent();
	BoundDetective->BroadcastCurrent();
}

void UGothamViewModelSubsystem::HandleHealth(float Health, float MaxHealth)
{
	Vitals->SetVitals(Health, MaxHealth);
}

void UGothamViewModelSubsystem::HandleGadgetCooldown(int32 Slot, float Remaining, float Total)
{
	if (UGadgetSlotViewModel* SlotVM = GadgetBar->GetSlot(Slot))
	{
		SlotVM->SetCooldown(Remaining, Total);
	}
}

void UGothamViewModelSubsystem::HandleCombo(int32 Hits, float Multiplier, float DecayAlpha)
{
	Combo->SetCombo(Hits, Multiplier, DecayAlpha);
}

/** One entry per clue placed in the level; scanning flips them to discovered. */
void UGothamViewModelSubsystem::RebuildClues()
{
	UDetectiveComponent* DetectiveComp = BoundDetective.Get();
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

void UGothamViewModelSubsystem::HandleDetective(bool bActive, float Alpha)
{
	Detective->SetState(bActive, Alpha);
}

void UGothamViewModelSubsystem::HandleClueScanned(const UClueDataAsset* Clue)
{
	if (Clue)
	{
		Clues->MarkDiscovered(Clue->ClueId);
		RefreshObjectives();
		ShowSubtitle(NSLOCTEXT("Gotham.Subtitles", "Detective", "Detective"),
			FText::Format(NSLOCTEXT("Gotham.Subtitles", "ClueFound", "{0}. {1}"), Clue->Title, Clue->Description), 5.f);
	}
}

void UGothamViewModelSubsystem::HandleSettings(const FGothamSettingsData& Data)
{
	Subtitles->SetPresentation(Data.GetSubtitleFontSize(), Data.bSubtitleBackground);
}

void UGothamViewModelSubsystem::ShowSubtitle(const FText& Speaker, const FText& Line, float Seconds)
{
	Subtitles->SetLine(Speaker, Line);
	FTSTicker::GetCoreTicker().RemoveTicker(SubtitleHideHandle);
	SubtitleHideHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		Subtitles->Clear();
		return false;
	}), Seconds);
}

void UGothamViewModelSubsystem::RefreshObjectives()
{
	int32 RealClues = 0;
	for (const UClueEntryViewModel* Entry : Clues->GetEntries())
	{
		RealClues += (Entry && !Entry->IsDebug()) ? 1 : 0;
	}
	// Debug clues are never discovered, so only the total needs filtering.
	Objectives->SetProgress(Clues->GetDiscoveredCount(), RealClues);
}

void UGothamViewModelSubsystem::OnCluesChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	RefreshObjectives();
}

void UGothamViewModelSubsystem::AddDebugClues(int32 Count)
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
