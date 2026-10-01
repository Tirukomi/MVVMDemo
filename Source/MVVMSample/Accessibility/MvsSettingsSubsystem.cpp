// Copyright IG. All Rights Reserved.

#include "Accessibility/MvsSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/ConfigCacheIni.h"
#include "ViewModels/SettingsViewModel.h"

namespace
{
	const TCHAR* ConfigSection = TEXT("/Script/MVVMSample.MvsSettings");
	const TCHAR* LegacyConfigSection = TEXT("/Script/MVVMSample.GothamSettings");
}

const TCHAR* UMvsSettingsSubsystem::GetConfigSection()
{
	return ConfigSection;
}

bool UMvsSettingsSubsystem::MigrateLegacySettings(FConfigFile& File)
{
	const FConfigSection* Legacy = File.FindSection(LegacyConfigSection);
	if (!Legacy || File.FindSection(ConfigSection))
	{
		return false;
	}
	const FConfigSection Values = *Legacy;
	for (const TPair<FName, FConfigValue>& Pair : Values)
	{
		File.AddToSection(ConfigSection, Pair.Key, Pair.Value.GetSavedValue());
	}
	File.Remove(LegacyConfigSection);
	File.Dirty = true;
	return true;
}

UMvsSettingsSubsystem* UMvsSettingsSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UMvsSettingsSubsystem>() : nullptr;
}

void UMvsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (FConfigFile* File = GConfig ? GConfig->FindConfigFile(GGameUserSettingsIni) : nullptr)
	{
		if (MigrateLegacySettings(*File))
		{
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		Live.LoadFromConfig(*File, ConfigSection);
	}

#if !UE_BUILD_SHIPPING
	// Dev aid for layout testing: -MvsLanguage=de overrides the saved language for this run only.
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("MvsLanguage="), Override) && !Override.IsEmpty())
	{
		Live.Language = Override;
	}
	// More dev overrides for screenshot runs: -MvsColorMode=0..3, -MvsUIScale=0..4, -MvsHighContrast, -MvsReducedMotion.
	int32 IntOverride = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("MvsColorMode="), IntOverride))
	{
		Live.ColorMode = static_cast<EMvsColorMode>(FMath::Clamp(IntOverride, 0, static_cast<int32>(EMvsColorMode::Count) - 1));
	}
	if (FParse::Value(FCommandLine::Get(), TEXT("MvsUIScale="), IntOverride))
	{
		Live.UIScaleIndex = FMath::Clamp(IntOverride, 0, FMvsSettingsData::GetUIScaleSteps().Num() - 1);
	}
	Live.bHighContrast |= FParse::Param(FCommandLine::Get(), TEXT("MvsHighContrast"));
	Live.bReducedMotion |= FParse::Param(FCommandLine::Get(), TEXT("MvsReducedMotion"));
#endif

	CultureBeforeGame = FInternationalization::Get().GetCurrentCulture()->GetName();
	ViewModel = NewObject<USettingsViewModel>(this);
	ViewModel->Initialize(Live);
	ViewModel->OnPreview.AddUObject(this, &UMvsSettingsSubsystem::HandlePreview);
	ViewModel->OnCommitted.AddUObject(this, &UMvsSettingsSubsystem::HandleCommitted);

	ApplyEffects(Live, true);
}

void UMvsSettingsSubsystem::Deinitialize()
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

void UMvsSettingsSubsystem::HandlePreview(const FMvsSettingsData& Data)
{
	const bool bLanguageChanged = Data.Language != Live.Language;
	Live = Data;
	ApplyEffects(Live, bLanguageChanged);
}

void UMvsSettingsSubsystem::HandleCommitted(const FMvsSettingsData& Data)
{
	Live = Data;
	if (FConfigFile* File = GConfig ? GConfig->FindConfigFile(GGameUserSettingsIni) : nullptr)
	{
		Live.SaveToConfig(*File, ConfigSection);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UMvsSettingsSubsystem::ApplyEffects(const FMvsSettingsData& Data, bool bLanguageChanged)
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
