// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamPlayerController.h"

#include "CommonActivatableWidget.h"
#include "Core/GothamCharacter.h"
#include "Core/GothamDevAids.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Input/GothamActionTable.h"
#include "Input/GothamUIInput.h"
#include "InputModifiers.h"
#include "PlayerMappableKeySettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "UI/GothamUISettings.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Gameplay/DetectiveComponent.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "ViewModels/GothamViewModelSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamHud, Log, All);

namespace
{
	UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name);
		Action->ValueType = Type;
		return Action;
	}

	/**
	 * Marks an action as player-rebindable under Name. All mappings of the action then become slots of one row
	 * (keyboard/mouse first, gamepad second). Enhanced Input keeps the settings object protected because it is
	 * normally authored on the asset; these actions are created in code, so it is set through reflection.
	 */
	void MakeActionMappable(UInputAction* Action, const FGothamActionDef& Def)
	{
		UPlayerMappableKeySettings* Settings = NewObject<UPlayerMappableKeySettings>(Action);
		Settings->Name = Def.Name;
		Settings->DisplayName = Def.DisplayName;
		if (FObjectProperty* Property = FindFProperty<FObjectProperty>(UInputAction::StaticClass(), TEXT("PlayerMappableKeySettings")))
		{
			Property->SetObjectPropertyValue_InContainer(Action, Settings);
		}
	}

	void MapNegatedY(UInputMappingContext* Context, UInputAction* Action, const FKey& Key)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Context);
		Negate->bX = false;
		Negate->bY = true;
		Negate->bZ = false;
		Context->MapKey(Action, Key).Modifiers.Add(Negate);
	}
}

void AGothamPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	RegisterRebindableContext();

	// Start level and slightly looking down, and keep pitch in a range that never puts the camera under the roof.
	SetControlRotation(FRotator(-12.f, GetControlRotation().Yaw, 0.f));
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -65.f;
		PlayerCameraManager->ViewPitchMax = 30.f;
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* UI = LocalPlayer->GetSubsystem<UGothamUISubsystem>())
		{
			UI->EnsureLayout(this);

			if (const TSubclassOf<UCommonActivatableWidget> HudClass = GetDefault<UGothamUISettings>()->HudScreenClass.LoadSynchronous())
			{
				UI->PushScreen(EGothamUILayer::Game, HudClass);
				UE_LOG(LogGothamHud, Log, TEXT("HUD screen pushed: %s"), *GetNameSafe(HudClass));
#if !UE_BUILD_SHIPPING
				GothamDevAids::Run(this, UI);
#endif
			}
			else
			{
				UE_LOG(LogGothamHud, Warning, TEXT("No HUD screen class configured in Gotham UI settings"));
			}
		}
	}
	BindViewModelsToPawn();
}

/**
 * Enhanced Input creates its user-settings object after SetupInputComponent runs, so the mapping context is
 * registered for rebinding here instead. Registering is what turns mappable actions into rebindable rows.
 */
void AGothamPlayerController::RegisterRebindableContext()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr;
	if (!UserSettings || !GameplayContext)
	{
		UE_LOG(LogGothamHud, Warning, TEXT("Enhanced Input user settings unavailable; controls cannot be rebound."));
		return;
	}
	const bool bRegistered = UserSettings->RegisterInputMappingContext(GameplayContext);
	int32 Mappable = 0;
	for (const FEnhancedActionKeyMapping& Mapping : GameplayContext->GetMappings())
	{
		Mappable += Mapping.IsPlayerMappable() ? 1 : 0;
	}
	UE_LOG(LogGothamHud, Log, TEXT("Input mapping context registered=%d, %d of %d mappings are player-mappable"),
		bRegistered ? 1 : 0, Mappable, GameplayContext->GetMappings().Num());
}

void AGothamPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	BuildInputAssets();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// Both stay on for the whole session: Common UI finds a menu action's keys through the active mappings, and
			// its Menu input mode blocks game input while a menu is open, so there is no context to swap.
			Input->AddMappingContext(GameplayContext, 0);
			Input->AddMappingContext(MenuContext, UGothamUIInputData::MappingPriority);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		const auto Action = [this](const TCHAR* Name) { return FindAction(Name); };
		EIC->BindAction(Action(TEXT("Move")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMove);
		EIC->BindAction(Action(TEXT("Look")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnLook);
		EIC->BindAction(Action(TEXT("MoveForward")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMoveDirection, FVector2D(0, 1));
		EIC->BindAction(Action(TEXT("MoveBack")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMoveDirection, FVector2D(0, -1));
		EIC->BindAction(Action(TEXT("MoveLeft")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMoveDirection, FVector2D(-1, 0));
		EIC->BindAction(Action(TEXT("MoveRight")), ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMoveDirection, FVector2D(1, 0));
		EIC->BindAction(Action(TEXT("Attack")), ETriggerEvent::Started, this, &AGothamPlayerController::OnAttack);
		EIC->BindAction(Action(TEXT("Counter")), ETriggerEvent::Started, this, &AGothamPlayerController::OnCounter);
		EIC->BindAction(Action(TEXT("Gadget1")), ETriggerEvent::Started, this, &AGothamPlayerController::OnGadget, 0);
		EIC->BindAction(Action(TEXT("Gadget2")), ETriggerEvent::Started, this, &AGothamPlayerController::OnGadget, 1);
		EIC->BindAction(Action(TEXT("Gadget3")), ETriggerEvent::Started, this, &AGothamPlayerController::OnGadget, 2);
		EIC->BindAction(Action(TEXT("Pause")), ETriggerEvent::Started, this, &AGothamPlayerController::OnPause);
		EIC->BindAction(Action(TEXT("GadgetWheel")), ETriggerEvent::Started, this, &AGothamPlayerController::OnGadgetWheel);
		EIC->BindAction(Action(TEXT("Detective")), ETriggerEvent::Started, this, &AGothamPlayerController::OnDetective);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Started, this, &AGothamPlayerController::OnScan);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Completed, this, &AGothamPlayerController::OnScanReleased);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Canceled, this, &AGothamPlayerController::OnScanReleased);
		EIC->BindAction(Action(TEXT("ClueLog")), ETriggerEvent::Started, this, &AGothamPlayerController::OnClueLog);
		EIC->BindAction(Action(TEXT("DebugDamage")), ETriggerEvent::Started, this, &AGothamPlayerController::OnDebugDamage);
		EIC->BindAction(Action(TEXT("DebugHeal")), ETriggerEvent::Started, this, &AGothamPlayerController::OnDebugHeal);
	}
}

void AGothamPlayerController::BuildInputAssets()
{
	GameplayContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Gameplay"));
	Actions.Reset();
	// Rebindable actions map their keyboard / mouse key first and their gamepad key second: those are the two slots.
	for (const FGothamActionDef& Def : GothamActions::GetTable())
	{
		UInputAction* Action = MakeAction(this, *FString::Printf(TEXT("IA_%s"), *Def.Name.ToString()), Def.ValueType);
		if (Def.bRebindable)
		{
			MakeActionMappable(Action, Def);
		}
		for (const FKey& Key : { Def.KeyboardKey, Def.GamepadKey })
		{
			if (!Key.IsValid())
			{
				continue;
			}
			if (Def.bInvertY)
			{
				MapNegatedY(GameplayContext, Action, Key);
			}
			else
			{
				GameplayContext->MapKey(Action, Key);
			}
		}
		Actions.Add(Def.Name, Action);
	}
	// Dev shortcut: F3 also attacks.
	GameplayContext->MapKey(Actions.FindRef(TEXT("Attack")), EKeys::F3);

	MenuContext = UGothamUIInputData::Get().BuildMappingContext(this);
}

void AGothamPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindViewModelsToPawn();
}

void AGothamPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	BindViewModelsToPawn();
}

void AGothamPlayerController::BindViewModelsToPawn()
{
	if (!IsLocalController())
	{
		return;
	}
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>())
		{
			ViewModels->BindToCharacter(Cast<AGothamCharacter>(GetPawn()));
		}
	}
}

void AGothamPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->MoveInput(Value.Get<FVector2D>());
	}
}

void AGothamPlayerController::OnMoveDirection(FVector2D Direction)
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->MoveInput(Direction);
	}
}

void AGothamPlayerController::OnLook(const FInputActionValue& Value)
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->LookInput(Value.Get<FVector2D>());
	}
}

void AGothamPlayerController::OnAttack()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->Attack();
	}
}

void AGothamPlayerController::OnCounter()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->Counter();
	}
}

void AGothamPlayerController::OnGadget(int32 SlotIndex)
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->UseGadget(SlotIndex);
	}
}

void AGothamPlayerController::OnDebugDamage()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->DebugDamage();
	}
}

void AGothamPlayerController::OnDebugHeal()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->DebugHeal();
	}
}

void AGothamPlayerController::OnPause()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->TogglePauseMenu();
	}
}

void AGothamPlayerController::OnDetective()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->ToggleDetective();
	}
}

void AGothamPlayerController::OnScan()
{
	AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn());
	if (!Hero)
	{
		return;
	}
	// Hold to analyse by default; the Tap accessibility option scans instantly.
	const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this);
	if (Settings && Settings->GetSettings().ScanMode == EGothamScanMode::Tap)
	{
		Hero->ScanClue();
	}
	else
	{
		Hero->GetDetectiveComponent()->BeginAnalyse();
	}
}

void AGothamPlayerController::OnScanReleased()
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetPawn()))
	{
		Hero->GetDetectiveComponent()->EndAnalyse();
	}
}

void AGothamPlayerController::OnClueLog()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->ToggleClueLog();
	}
}

void AGothamPlayerController::OnGadgetWheel()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->OpenGadgetWheel();
	}
}

const UInputAction* AGothamPlayerController::FindAction(FName Name) const
{
	const TObjectPtr<UInputAction>* Found = Actions.Find(Name);
	return Found ? Found->Get() : nullptr;
}
