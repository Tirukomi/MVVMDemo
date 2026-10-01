// Copyright Epic Games, Inc. All Rights Reserved.

#include "Accessibility/GothamSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

namespace
{
	const TCHAR* ConfigSection = TEXT("/Script/MVVMSample.GothamSettings");
}

UGothamSettingsSubsystem* UGothamSettingsSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UGothamSettingsSubsystem>() : nullptr;
}

void UGothamSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const FConfigFile* File = GConfig ? GConfig->FindConfigFile(GGameUserSettingsIni) : nullptr)
	{
		Live.LoadFromConfig(*File, ConfigSection);
	}

#if !UE_BUILD_SHIPPING
	// Dev aid for layout testing: -GothamLanguage=de overrides the saved language for this run only.
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("GothamLanguage="), Override) && !Override.IsEmpty())
	{
		Live.Language = Override;
	}
	// More dev overrides for screenshot runs: -GothamColorMode=0..3, -GothamUIScale=0..4, -GothamHighContrast, -GothamReducedMotion.
	int32 IntOverride = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("GothamColorMode="), IntOverride))
	{
		Live.ColorMode = static_cast<EGothamColorMode>(FMath::Clamp(IntOverride, 0, static_cast<int32>(EGothamColorMode::Count) - 1));
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("GothamUIScale="), IntOverride))
	{
		Live.UIScaleIndex = FMath::Clamp(IntOverride, 0, FGothamSettingsData::GetUIScaleSteps().Num() - 1);
	}
	Live.bHighContrast |= FParse::Param(FCommandLine::Get(), TEXT("GothamHighContrast"));
	Live.bReducedMotion |= FParse::Param(FCommandLine::Get(), TEXT("GothamReducedMotion"));
#endif

	CultureBeforeGame = FInternationalization::Get().GetCurrentCulture()->GetName();
	ViewModel = NewObject<USettingsViewModel>(this);
	ViewModel->Initialize(Live);
	ViewModel->OnPreview.AddUObject(this, &UGothamSettingsSubsystem::HandlePreview);
	ViewModel->OnCommitted.AddUObject(this, &UGothamSettingsSubsystem::HandleCommitted);

	ApplyEffects(Live, true);
}

void UGothamSettingsSubsystem::Deinitialize()
{
	if (ViewModel)
	{
		ViewModel->OnPreview.RemoveAll(this);
		ViewModel->OnCommitted.RemoveAll(this);
	}
	if (!CultureBeforeGame.IsEmpty() && FInternationalization::Get().GetCurrentCulture()->GetName() != CultureBeforeGame)
	{
		FInternationalization::Get().SetCurrentCulture(CultureBeforeGame);
	}
	Super::Deinitialize();
}

void UGothamSettingsSubsystem::HandlePreview(const FGothamSettingsData& Data)
{
	const bool bLanguageChanged = Data.Language != Live.Language;
	Live = Data;
	ApplyEffects(Live, bLanguageChanged);
}

void UGothamSettingsSubsystem::HandleCommitted(const FGothamSettingsData& Data)
{
	Live = Data;
	if (FConfigFile* File = GConfig ? GConfig->FindConfigFile(GGameUserSettingsIni) : nullptr)
	{
		Live.SaveToConfig(*File, ConfigSection);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UGothamSettingsSubsystem::ApplyEffects(const FGothamSettingsData& Data, bool bLanguageChanged)
{
	if (bLanguageChanged)
	{
		FInternationalization::Get().SetCurrentCulture(Data.Language);
		if (ViewModel)
		{
			ViewModel->RefreshTexts();
		}
	}
	// UI scale is applied by the primary layout (a DPI scaler around every layer), from this broadcast. It used to be
	// written to UUserInterfaceSettings' class default object, which outlived a play-in-editor session.
	OnSettingsChanged.Broadcast(Live);
}
