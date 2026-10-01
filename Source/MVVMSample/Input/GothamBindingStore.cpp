// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamBindingStore.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamBindings, Log, All);

namespace
{
	/** Enhanced Input's user settings: the active key profile holds one row per player-mappable action. */
	class FEnhancedInputBindingStore final : public IGothamBindingStore
	{
	public:
		explicit FEnhancedInputBindingStore(ULocalPlayer* InPlayer) : Player(InPlayer) {}

		virtual TArray<FGothamBindingSlot> GetBindings() const override
		{
			TArray<FGothamBindingSlot> Bindings;
			const UEnhancedInputUserSettings* UserSettings = GetUserSettings();
			const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
			if (!Profile)
			{
				UE_LOG(LogGothamBindings, Warning, TEXT("No Enhanced Input key profile (is bEnableUserSettings on?); controls cannot be shown."));
				return Bindings;
			}
			for (const TPair<FName, FKeyMappingRow>& Pair : Profile->GetPlayerMappingRows())
			{
				for (const FPlayerKeyMapping& Mapping : Pair.Value.Mappings)
				{
					Bindings.Add({ Mapping.GetMappingName(), static_cast<int32>(Mapping.GetSlot()), Mapping.GetCurrentKey() });
				}
			}
			return Bindings;
		}

		virtual void Apply(const TArray<FGothamBindingChange>& Changes) override
		{
			UEnhancedInputUserSettings* UserSettings = GetUserSettings();
			if (!UserSettings)
			{
				return;
			}
			for (const FGothamBindingChange& Change : Changes)
			{
				FMapPlayerKeyArgs Args;
				Args.MappingName = Change.Name;
				Args.Slot = static_cast<EPlayerMappableKeySlot>(Change.Slot);
				Args.NewKey = Change.NewKey;
				FGameplayTagContainer Failure;
				UserSettings->MapPlayerKey(Args, Failure);
			}
			UserSettings->ApplySettings();
			UserSettings->SaveSettings();
		}

		virtual void ResetToDefaults() override
		{
			UEnhancedInputUserSettings* UserSettings = GetUserSettings();
			UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
			if (!Profile)
			{
				return;
			}
			Profile->ResetToDefault();
			UserSettings->ApplySettings();
			UserSettings->SaveSettings();
		}

	private:
		UEnhancedInputUserSettings* GetUserSettings() const
		{
			const ULocalPlayer* LocalPlayer = Player.Get();
			const auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
			return Input ? Input->GetUserSettings() : nullptr;
		}

		TWeakObjectPtr<ULocalPlayer> Player;
	};
}

TSharedRef<IGothamBindingStore> GothamBindings::MakeEnhancedInputStore(ULocalPlayer* Player)
{
	return MakeShared<FEnhancedInputBindingStore>(Player);
}
