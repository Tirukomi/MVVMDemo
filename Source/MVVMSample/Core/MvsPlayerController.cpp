// Copyright IG. All Rights Reserved.

#include "Core/MvsPlayerController.h"

#include "CommonActivatableWidget.h"
#include "Core/MvsCharacter.h"
#include "Core/MvsDevAids.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Input/MvsActionTable.h"
#include "Input/MvsUIInput.h"
#include "InputModifiers.h"
#include "PlayerMappableKeySettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "UI/MvsUISettings.h"
#include "Accessibility/MvsSettingsSubsystem.h"
#include "Gameplay/ForensicComponent.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "ViewModels/MvsViewModelSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsHud, Log, All);

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
	void MakeActionMappable(UInputAction* Action, const FMvsActionDef& Def)
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

void AMvsPlayerController::BeginPlay()
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
		if (auto* UI = LocalPlayer->GetSubsystem<UMvsUISubsystem>())
		{
			UI->EnsureLayout(this);

			// The HUD is needed right now, at level start, so it is the one class loaded synchronously.
			if (const TSubclassOf<UCommonActivatableWidget> HudClass = GetDefault<UMvsUISettings>()->HudScreenClass.LoadSynchronous())
			{
				UI->PushScreen(EMvsUILayer::Game, HudClass);
				UE_LOG(LogMvsHud, Log, TEXT("HUD screen pushed: %s"), *GetNameSafe(HudClass));
#if !UE_BUILD_SHIPPING
				MvsDevAids::Run(this, UI);
#endif
			}
			else
			{
				UE_LOG(LogMvsHud, Warning, TEXT("No HUD screen class configured in Mvs UI settings"));
			}
		}
	}
	BindViewModelsToPawn();
}

/**
 * Enhanced Input creates its user-settings object after SetupInputComponent runs, so the mapping context is
 * registered for rebinding here instead. Registering is what turns mappable actions into rebindable rows.
 */
void AMvsPlayerController::RegisterRebindableContext()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr;
	if (!UserSettings || !GameplayContext)
	{
		UE_LOG(LogMvsHud, Warning, TEXT("Enhanced Input user settings unavailable; controls cannot be rebound."));
		return;
	}
	const bool bRegistered = UserSettings->RegisterInputMappingContext(GameplayContext);
	int32 Mappable = 0;
	for (const FEnhancedActionKeyMapping& Mapping : GameplayContext->GetMappings())
	{
		Mappable += Mapping.IsPlayerMappable() ? 1 : 0;
	}
	UE_LOG(LogMvsHud, Log, TEXT("Input mapping context registered=%d, %d of %d mappings are player-mappable"),
		bRegistered ? 1 : 0, Mappable, GameplayContext->GetMappings().Num());
}

void AMvsPlayerController::SetupInputComponent()
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
			Input->AddMappingContext(MenuContext, UMvsUIInputData::MappingPriority);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		const auto Action = [this](const TCHAR* Name) { return FindAction(Name); };
		EIC->BindAction(Action(TEXT("Move")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnMove);
		EIC->BindAction(Action(TEXT("Look")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnLook);
		EIC->BindAction(Action(TEXT("MoveForward")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnMoveDirection, FVector2D(0, 1));
		EIC->BindAction(Action(TEXT("MoveBack")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnMoveDirection, FVector2D(0, -1));
		EIC->BindAction(Action(TEXT("MoveLeft")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnMoveDirection, FVector2D(-1, 0));
		EIC->BindAction(Action(TEXT("MoveRight")), ETriggerEvent::Triggered, this, &AMvsPlayerController::OnMoveDirection, FVector2D(1, 0));
		EIC->BindAction(Action(TEXT("Attack")), ETriggerEvent::Started, this, &AMvsPlayerController::OnAttack);
		EIC->BindAction(Action(TEXT("Counter")), ETriggerEvent::Started, this, &AMvsPlayerController::OnCounter);
		EIC->BindAction(Action(TEXT("Gadget1")), ETriggerEvent::Started, this, &AMvsPlayerController::OnGadget, 0);
		EIC->BindAction(Action(TEXT("Gadget2")), ETriggerEvent::Started, this, &AMvsPlayerController::OnGadget, 1);
		EIC->BindAction(Action(TEXT("Gadget3")), ETriggerEvent::Started, this, &AMvsPlayerController::OnGadget, 2);
		EIC->BindAction(Action(TEXT("Pause")), ETriggerEvent::Started, this, &AMvsPlayerController::OnPause);
		EIC->BindAction(Action(TEXT("GadgetWheel")), ETriggerEvent::Started, this, &AMvsPlayerController::OnGadgetWheel);
		EIC->BindAction(Action(TEXT("Forensic")), ETriggerEvent::Started, this, &AMvsPlayerController::OnForensic);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Started, this, &AMvsPlayerController::OnScan);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Completed, this, &AMvsPlayerController::OnScanReleased);
		EIC->BindAction(Action(TEXT("Scan")), ETriggerEvent::Canceled, this, &AMvsPlayerController::OnScanReleased);
		EIC->BindAction(Action(TEXT("ClueLog")), ETriggerEvent::Started, this, &AMvsPlayerController::OnClueLog);
		EIC->BindAction(Action(TEXT("DebugDamage")), ETriggerEvent::Started, this, &AMvsPlayerController::OnDebugDamage);
		EIC->BindAction(Action(TEXT("DebugHeal")), ETriggerEvent::Started, this, &AMvsPlayerController::OnDebugHeal);
	}
}

void AMvsPlayerController::BuildInputAssets()
{
	GameplayContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Gameplay"));
	Actions.Reset();
	// Rebindable actions map their keyboard / mouse key first and their gamepad key second: those are the two slots.
	for (const FMvsActionDef& Def : MvsActions::GetTable())
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

	MenuContext = UMvsUIInputData::Get().BuildMappingContext(this);
}

void AMvsPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindViewModelsToPawn();
}

void AMvsPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	BindViewModelsToPawn();
}

void AMvsPlayerController::BindViewModelsToPawn()
{
	if (!IsLocalController())
	{
		return;
	}
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>())
		{
			ViewModels->BindToCharacter(Cast<AMvsCharacter>(GetPawn()));
		}
	}
}

void AMvsPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->MoveInput(Value.Get<FVector2D>());
	}
}

void AMvsPlayerController::OnMoveDirection(FVector2D Direction)
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->MoveInput(Direction);
	}
}

void AMvsPlayerController::OnLook(const FInputActionValue& Value)
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->LookInput(Value.Get<FVector2D>());
	}
}

void AMvsPlayerController::OnAttack()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->Attack();
	}
}

void AMvsPlayerController::OnCounter()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->Counter();
	}
}

void AMvsPlayerController::OnGadget(int32 SlotIndex)
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->UseGadget(SlotIndex);
	}
}

void AMvsPlayerController::OnDebugDamage()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->DebugDamage();
	}
}

void AMvsPlayerController::OnDebugHeal()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->DebugHeal();
	}
}

void AMvsPlayerController::OnPause()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->TogglePauseMenu();
	}
}

void AMvsPlayerController::OnForensic()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->ToggleForensic();
	}
}

void AMvsPlayerController::OnScan()
{
	AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn());
	if (!Hero)
	{
		return;
	}
	// Hold to analyse by default; the Tap accessibility option scans instantly.
	const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this);
	if (Settings && Settings->GetSettings().ScanMode == EMvsScanMode::Tap)
	{
		Hero->ScanClue();
	}
	else
	{
		Hero->GetForensicComponent()->BeginAnalyse();
	}
}

void AMvsPlayerController::OnScanReleased()
{
	if (AMvsCharacter* Hero = Cast<AMvsCharacter>(GetPawn()))
	{
		Hero->GetForensicComponent()->EndAnalyse();
	}
}

void AMvsPlayerController::OnClueLog()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->ToggleClueLog();
	}
}

void AMvsPlayerController::OnGadgetWheel()
{
	if (auto* UI = GetLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->OpenGadgetWheel();
	}
}

const UInputAction* AMvsPlayerController::FindAction(FName Name) const
{
	const TObjectPtr<UInputAction>* Found = Actions.Find(Name);
	return Found ? Found->Get() : nullptr;
}
