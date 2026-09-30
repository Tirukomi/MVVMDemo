// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamPlayerController.h"

#include "CommonActivatableWidget.h"
#include "Core/GothamCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Input/GothamBindings.h"
#include "InputModifiers.h"
#include "PlayerMappableKeySettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Core/GothamCharacter.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/HealthComponent.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/GothamUISettings.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/GothamMenuInputTest.h"
#include "Core/GothamPerfHarness.h"
#include "ViewModels/SettingsViewModel.h"
#include "EngineUtils.h"
#include "Gameplay/ClueActor.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/ClueEntryWidget.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UnrealClient.h"
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
	void MakeActionMappable(UInputAction* Action, FName Name)
	{
		UPlayerMappableKeySettings* Settings = NewObject<UPlayerMappableKeySettings>(Action);
		Settings->Name = Name;
		for (const FGothamBindingDef& Def : GothamBindings::GetDefinitions())
		{
			if (Def.Name == Name)
			{
				Settings->DisplayName = Def.DisplayName;
			}
		}
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
			UI->OnInputContextChanged.AddUObject(this, &AGothamPlayerController::ApplyInputContext);

			if (const TSubclassOf<UCommonActivatableWidget> HudClass = GetDefault<UGothamUISettings>()->HudScreenClass.LoadSynchronous())
			{
				UI->PushScreen(EGothamUILayer::Game, HudClass);
				UE_LOG(LogGothamHud, Log, TEXT("HUD screen pushed: %s"), *GetNameSafe(HudClass));
#if !UE_BUILD_SHIPPING
				RunDevAids(UI);
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
			Input->AddMappingContext(GameplayContext, 0);
		}
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMove);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGothamPlayerController::OnLook);
		for (int32 i = 0; i < MoveDirectionActions.Num(); ++i)
		{
			EIC->BindAction(MoveDirectionActions[i], ETriggerEvent::Triggered, this, &AGothamPlayerController::OnMoveDirection, MoveDirections[i]);
		}
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnAttack);
		EIC->BindAction(CounterAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnCounter);
		for (int32 i = 0; i < GadgetActions.Num(); ++i)
		{
			EIC->BindAction(GadgetActions[i], ETriggerEvent::Started, this, &AGothamPlayerController::OnGadget, i);
		}
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnPause);
		EIC->BindAction(GadgetWheelAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnGadgetWheel);
		EIC->BindAction(DetectiveAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnDetective);
		EIC->BindAction(ScanAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnScan);
		EIC->BindAction(ScanAction, ETriggerEvent::Completed, this, &AGothamPlayerController::OnScanReleased);
		EIC->BindAction(ScanAction, ETriggerEvent::Canceled, this, &AGothamPlayerController::OnScanReleased);
		EIC->BindAction(ClueLogAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnClueLog);
		EIC->BindAction(DebugDamageAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnDebugDamage);
		EIC->BindAction(DebugHealAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnDebugHeal);
	}
}

void AGothamPlayerController::BuildInputAssets()
{
	GameplayContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Gameplay"));
	MoveAction = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	AttackAction = MakeAction(this, TEXT("IA_Attack"), EInputActionValueType::Boolean);
	CounterAction = MakeAction(this, TEXT("IA_Counter"), EInputActionValueType::Boolean);
	DebugDamageAction = MakeAction(this, TEXT("IA_DebugDamage"), EInputActionValueType::Boolean);
	DebugHealAction = MakeAction(this, TEXT("IA_DebugHeal"), EInputActionValueType::Boolean);
	PauseAction = MakeAction(this, TEXT("IA_Pause"), EInputActionValueType::Boolean);
	GadgetWheelAction = MakeAction(this, TEXT("IA_GadgetWheel"), EInputActionValueType::Boolean);
	DetectiveAction = MakeAction(this, TEXT("IA_Detective"), EInputActionValueType::Boolean);
	ScanAction = MakeAction(this, TEXT("IA_Scan"), EInputActionValueType::Boolean);
	ClueLogAction = MakeAction(this, TEXT("IA_ClueLog"), EInputActionValueType::Boolean);

	// Move: WASD, one rebindable action per direction, plus the left stick (fixed).
	const struct { const TCHAR* Name; FKey Key; FVector2D Direction; } MoveKeys[] = {
		{ TEXT("MoveForward"), EKeys::W, FVector2D(0, 1) },
		{ TEXT("MoveBack"), EKeys::S, FVector2D(0, -1) },
		{ TEXT("MoveLeft"), EKeys::A, FVector2D(-1, 0) },
		{ TEXT("MoveRight"), EKeys::D, FVector2D(1, 0) },
	};
	for (const auto& Move : MoveKeys)
	{
		UInputAction* Action = MakeAction(this, *FString::Printf(TEXT("IA_%s"), Move.Name), EInputActionValueType::Boolean);
		MakeActionMappable(Action, Move.Name);
		GameplayContext->MapKey(Action, Move.Key);
		MoveDirectionActions.Add(Action);
		MoveDirections.Add(Move.Direction);
	}
	GameplayContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Look: mouse + right stick (fixed).
	MapNegatedY(GameplayContext, LookAction, EKeys::Mouse2D);
	MapNegatedY(GameplayContext, LookAction, EKeys::Gamepad_Right2D);

	// Every rebindable action gets its keyboard/mouse key first and its gamepad button second (the two slots).
	auto MapPair = [this](UInputAction* Action, const TCHAR* Name, const FKey& Keyboard, const FKey& Pad)
	{
		MakeActionMappable(Action, Name);
		GameplayContext->MapKey(Action, Keyboard);
		GameplayContext->MapKey(Action, Pad);
	};

	MapPair(AttackAction, TEXT("Attack"), EKeys::LeftMouseButton, EKeys::Gamepad_FaceButton_Bottom);
	MapPair(CounterAction, TEXT("Counter"), EKeys::RightMouseButton, EKeys::Gamepad_RightShoulder);

	const FKey GadgetKeys[] = { EKeys::One, EKeys::Two, EKeys::Three };
	const FKey GadgetPadKeys[] = { EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_FaceButton_Right };
	for (int32 i = 0; i < 3; ++i)
	{
		UInputAction* Action = MakeAction(this, *FString::Printf(TEXT("IA_Gadget%d"), i + 1), EInputActionValueType::Boolean);
		MapPair(Action, *FString::Printf(TEXT("Gadget%d"), i + 1), GadgetKeys[i], GadgetPadKeys[i]);
		GadgetActions.Add(Action);
	}

	MapPair(DetectiveAction, TEXT("Detective"), EKeys::V, EKeys::Gamepad_DPad_Up);
	MapPair(ScanAction, TEXT("Scan"), EKeys::E, EKeys::Gamepad_DPad_Right);
	MapPair(ClueLogAction, TEXT("ClueLog"), EKeys::J, EKeys::Gamepad_Special_Left);
	MapPair(GadgetWheelAction, TEXT("GadgetWheel"), EKeys::Q, EKeys::Gamepad_LeftShoulder);
	MapPair(PauseAction, TEXT("Pause"), EKeys::Escape, EKeys::Gamepad_Special_Right);

	GameplayContext->MapKey(DebugDamageAction, EKeys::F1);
	GameplayContext->MapKey(DebugHealAction, EKeys::F2);
	GameplayContext->MapKey(AttackAction, EKeys::F3);
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
	if (Name == TEXT("Move")) return MoveAction;
	if (Name == TEXT("Look")) return LookAction;
	if (Name == TEXT("Attack")) return AttackAction;
	if (Name == TEXT("Counter")) return CounterAction;
	if (Name == TEXT("Pause")) return PauseAction;
	if (Name == TEXT("GadgetWheel")) return GadgetWheelAction;
	if (Name == TEXT("Detective")) return DetectiveAction;
	if (Name == TEXT("Scan")) return ScanAction;
	if (Name == TEXT("ClueLog")) return ClueLogAction;
	if (Name == TEXT("Gadget1")) return GadgetActions.IsValidIndex(0) ? GadgetActions[0].Get() : nullptr;
	if (Name == TEXT("Gadget2")) return GadgetActions.IsValidIndex(1) ? GadgetActions[1].Get() : nullptr;
	if (Name == TEXT("Gadget3")) return GadgetActions.IsValidIndex(2) ? GadgetActions[2].Get() : nullptr;
	return nullptr;
}

/** The gameplay mapping context is live only while no menu owns input. */
void AGothamPlayerController::ApplyInputContext(EGothamInputContext Context)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Input)
	{
		return;
	}
	if (Context == EGothamInputContext::Gameplay)
	{
		Input->AddMappingContext(GameplayContext, 0);
	}
	else
	{
		Input->RemoveMappingContext(GameplayContext);
	}
}

#if !UE_BUILD_SHIPPING
/**
 * Dev aids for headless verification, enabled by command-line flags. Each saves a screenshot after 4s.
 *   -GothamOpenPause     opens the pause menu
 *   -GothamOpenQuit      opens the pause menu, then its (destructive) quit confirmation
 *   -GothamMenuInputTest toggle-key and clickable-prompt checks through Slate input; logs PASS / FAIL, then quits
 *   -GothamCombatDemo    a thug in view and one behind telegraph at once (prompt + arrow), with a 10-hit combo
 *   Thugs never start attacks on their own during these runs (except -GothamCombatDemo's forced ones).
 *   -GothamOpenWheel     opens the gadget wheel, hovers a segment and builds a combo
 *   -GothamDetective     enters detective mode and scans the nearest clue
 *   -GothamClueLog[=N]   scans a clue, opens the case file, optionally with N extra fake clues
 *   -GothamCycleLanguage steps the language once after 2s (with -GothamOpenSettings: a live switch)
 *   -GothamShotDelay=S   seconds before the screenshot (default 4; raise it on a cold shader cache)
 */
void AGothamPlayerController::RunDevAids(UGothamUISubsystem* UI)
{
	const TCHAR* Cmd = FCommandLine::Get();
	// Screenshot runs must be deterministic: the first mouse delta after window capture would otherwise swing the camera.
	if (FParse::Param(Cmd, TEXT("GothamShot")) || FParse::Param(Cmd, TEXT("GothamOpenWheel")) || FParse::Param(Cmd, TEXT("GothamDetective"))
		|| FParse::Param(Cmd, TEXT("GothamOpenPause")) || FParse::Param(Cmd, TEXT("GothamOpenQuit")) || FParse::Param(Cmd, TEXT("GothamOpenSettings")) || FParse::Param(Cmd, TEXT("GothamClueLog"))
		|| FParse::Param(Cmd, TEXT("GothamHudDemo")) || FParse::Param(Cmd, TEXT("GothamCombatDemo")))
	{
		SetIgnoreLookInput(true);
	}
	const bool bQuit = FParse::Param(Cmd, TEXT("GothamOpenQuit"));
	const bool bPause = FParse::Param(Cmd, TEXT("GothamOpenPause")) || bQuit;
	const bool bWheel = FParse::Param(Cmd, TEXT("GothamOpenWheel"));
	const bool bDetective = FParse::Param(Cmd, TEXT("GothamDetective"));
	int32 StressCount = 0;
	FParse::Value(Cmd, TEXT("GothamClueLog="), StressCount);
	// "-GothamClueLog" and "-GothamClueLog=N" both open the case file.
	const bool bClueLog = FParse::Param(Cmd, TEXT("GothamClueLog")) || StressCount > 0;
	const bool bSettings = FParse::Param(Cmd, TEXT("GothamOpenSettings"));
	const bool bControls = FParse::Param(Cmd, TEXT("GothamOpenControls"));
	const bool bCycleLanguage = FParse::Param(Cmd, TEXT("GothamCycleLanguage"));
	const bool bRebindDemo = FParse::Param(Cmd, TEXT("GothamRebindDemo"));
	const bool bReveal = FParse::Param(Cmd, TEXT("GothamDetectiveReveal"));
	const bool bAnalyse = FParse::Param(Cmd, TEXT("GothamDetectiveAnalyse"));
	const bool bHudDemo = FParse::Param(Cmd, TEXT("GothamHudDemo"));
	const bool bCombatDemo = FParse::Param(Cmd, TEXT("GothamCombatDemo"));
	const bool bPlainShot = FParse::Param(Cmd, TEXT("GothamShot")) || bHudDemo || bCombatDemo;

	// -GothamMenuInputTest drives the menus through Slate input and logs PASS / FAIL per rule, then quits.
	if (FParse::Param(Cmd, TEXT("GothamMenuInputTest")))
	{
		FGothamMenuInputTest::Start(this);
		return;
	}

	// -GothamPerf=<label> runs the UI performance harness and quits (see Docs/Performance.md).
	FString PerfLabel;
	if (FParse::Value(Cmd, TEXT("GothamPerf="), PerfLabel) && !PerfLabel.IsEmpty())
	{
		FGothamPerfHarness::Start(this, PerfLabel);
		return;
	}
	if (!(bPause || bWheel || bDetective || bClueLog || bSettings || bControls || bCycleLanguage || bRebindDemo || bPlainShot || StressCount > 0))
	{
		return;
	}
	// Screenshots must be reproducible: no thug picks a random moment to attack (and flash the vignette) mid-shot.
	if (UGothamThreatSubsystem* Threats = GetWorld()->GetSubsystem<UGothamThreatSubsystem>())
	{
		Threats->SetDirectorEnabled(false);
	}

	if (bPause)
	{
		UI->TogglePauseMenu();
	}
	if (bQuit)
	{
		// After the pause menu's intro, the way a player would reach it.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			for (TObjectIterator<UPauseMenuScreen> It; It; ++It)
			{
				if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
				{
					It->RequestQuit();
				}
			}
			return false;
		}), 1.f);
	}
	if (bWheel)
	{
		UI->OpenGadgetWheel();
	}
	if (bSettings)
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass.LoadSynchronous());
	}
	if (bControls)
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass.LoadSynchronous());
	}
	const FString ShotName = bPlainShot ? TEXT("gotham_hud") : bQuit ? TEXT("gotham_quit") : bRebindDemo ? TEXT("gotham_controls") : bPause ? TEXT("gotham_pause") : bWheel ? TEXT("gotham_wheel") : bDetective ? TEXT("gotham_detective") : bSettings ? TEXT("gotham_settings") : bControls ? TEXT("gotham_controls") : TEXT("gotham_cluelog");
	const TWeakObjectPtr<AGothamPlayerController> WeakThis(this);
	const TWeakObjectPtr<UGothamUISubsystem> WeakUI(UI);

	// Core tickers, not world timers: the world is paused or slowed while some of these screens are open.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakUI, bWheel, bDetective, bClueLog, StressCount, bReveal, bAnalyse](float)
	{
		AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr;
		if (!Hero)
		{
			return false;
		}
		if (bWheel)
		{
			for (TObjectIterator<UGadgetWheelScreen> It; It; ++It)
			{
				It->SetStickInput(FVector2D(0.9, 0.3)); // hover the right-hand segment
			}
			for (int32 i = 0; i < 7; ++i)
			{
				Hero->GetComboComponent()->RegisterHit();
			}
		}
		if (bDetective || bClueLog)
		{
			// Stand a few metres from the first clue, facing it and the rest of the roof.
			for (TActorIterator<AClueActor> It(Hero->GetWorld()); It; ++It)
			{
				const FVector Clue = It->GetActorLocation();
				Hero->SetActorLocation(Clue + FVector(-420.f, -260.f, 40.f));
				WeakThis->SetControlRotation(FRotator(-14.f, (Clue - Hero->GetActorLocation()).Rotation().Yaw + 12.f, 0.f));
				break;
			}
			// -GothamDetectiveReveal / -GothamDetectiveAnalyse open the mode later, so the shot lands mid-effect.
			if (!bReveal && !bAnalyse)
			{
				Hero->ToggleDetective();
				Hero->ScanClue();
			}
		}
		if ((bClueLog || StressCount > 0) && WeakUI.IsValid())
		{
			if (StressCount > 0)
			{
				if (auto* ViewModels = WeakThis->GetLocalPlayer()->GetSubsystem<UGothamViewModelSubsystem>())
				{
					ViewModels->AddDebugClues(StressCount);
				}
			}
			WeakUI->ToggleClueLog();
		}
		return false;
	}), 1.f);

	if (bDetective && (bReveal || bAnalyse))
	{
		float Delay = 4.f;
		FParse::Value(Cmd, TEXT("GothamShotDelay="), Delay);
		// Reveal: open 0.8 s before the shot (the opening pulse is ~half way). Analyse: open earlier, start analysing
		// 0.55 s before the shot (about half of the 1.1 s analysis).
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr)
			{
				Hero->ToggleDetective();
			}
			return false;
		}), bReveal ? Delay - 0.8f : Delay - 2.f);
		if (bAnalyse)
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
			{
				if (AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr)
				{
					Hero->GetDetectiveComponent()->BeginAnalyse();
				}
				return false;
			}), Delay - 0.55f);
		}
	}
	if (bHudDemo)
	{
		// A representative combat moment: a recent hit, a live combo and a gadget recharging.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr)
			{
				Hero->GetHealthComponent()->ApplyDamage(38.f);
				Hero->UseGadget(1);
			}
			return false;
		}), 5.f);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr)
			{
				Hero->GetHealthComponent()->ApplyDamage(9.f); // lands just before the screenshot, so ghost and flash show
				for (int32 i = 0; i < 12; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
			return false;
		}), 7.85f);
	}
	if (bCombatDemo)
	{
		// Forced telegraphs timed so the screenshot lands mid-warning: one thug in view (counter prompt) and the one
		// most behind the camera (a red edge arrow). The combo crosses 10 just before, so the callout shows too.
		float Delay = 4.f;
		FParse::Value(Cmd, TEXT("GothamShotDelay="), Delay);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			UWorld* World = WeakThis.IsValid() ? WeakThis->GetWorld() : nullptr;
			if (UGothamThreatSubsystem* Threats = World ? World->GetSubsystem<UGothamThreatSubsystem>() : nullptr)
			{
				Threats->ForceWarningOnVisible();
				Threats->ForceWarningBehind();
			}
			return false;
		}), Delay - 0.45f);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (AGothamCharacter* Hero = WeakThis.IsValid() ? Cast<AGothamCharacter>(WeakThis->GetPawn()) : nullptr)
			{
				for (int32 i = 0; i < 10; ++i)
				{
					Hero->GetComboComponent()->RegisterHit();
				}
			}
			return false;
		}), Delay - 0.6f);
	}
	if (bRebindDemo)
	{
		// Rebinds Scan (keyboard slot) from E to R through Enhanced Input user settings, then opens the controls
		// screen so the result is visible. Exercises the same MapPlayerKey and save path the screen uses.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis, WeakUI](float)
		{
			if (!WeakThis.IsValid() || !WeakUI.IsValid())
			{
				return false;
			}
			auto* Input = WeakThis->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr)
			{
				FMapPlayerKeyArgs Args;
				Args.MappingName = TEXT("Scan");
				Args.Slot = EPlayerMappableKeySlot::First;
				Args.NewKey = EKeys::R;
				FGameplayTagContainer Failure;
				UserSettings->MapPlayerKey(Args, Failure);
				UserSettings->ApplySettings();
				// Saved like a rebind made on the Controls screen, so a restart shows whether it loads back.
				UserSettings->SaveSettings();
				UE_LOG(LogGothamHud, Log, TEXT("Rebind demo: Scan -> R (failure tags: %d)"), Failure.Num());
			}
			WeakUI->NotifyBindingsChanged();
			WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass.LoadSynchronous());
			return false;
		}), 1.5f);
	}
	if (bCycleLanguage)
	{
		// Proves live language switching: the settings screen is already open when the language changes.
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis](float)
		{
			if (WeakThis.IsValid())
			{
				if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(WeakThis.Get()))
				{
					Settings->GetViewModel()->Cycle(EGothamSetting::Language, 1);
				}
			}
			return false;
		}), 2.f);
	}

	// Later than the actions above so shaders (compiled on first run) and transitions have settled.
	float ShotDelay = 4.f;
	FParse::Value(Cmd, TEXT("GothamShotDelay="), ShotDelay);
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([ShotName, StressCount, WeakThis](float)
	{
		if (StressCount > 0)
		{
			// The objective tracks the level's real clues only; the fake ones exist just to stress the list.
			const ULocalPlayer* LocalPlayer = WeakThis.IsValid() ? WeakThis->GetLocalPlayer() : nullptr;
			if (const UGothamViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr)
			{
				UE_LOG(LogGothamHud, Log, TEXT("Objective: %d / %d (clue list holds %d entries)"),
					ViewModels->GetObjectives()->GetFoundCount(), ViewModels->GetObjectives()->GetTotalCount(), ViewModels->GetClues()->GetTotalCount());
			}
			// Proof of virtualisation: rows alive should stay near the viewport size, not the item count.
			int32 Rows = 0;
			for (TObjectIterator<UClueEntryWidget> It; It; ++It)
			{
				Rows += It->HasAnyFlags(RF_ClassDefaultObject) ? 0 : 1;
			}
			UE_LOG(LogGothamHud, Log, TEXT("Clue log: %d row widgets alive for %d fake clues"), Rows, StressCount);
		}
		FScreenshotRequest::RequestScreenshot(ShotName, true, false);
		return false;
	}), ShotDelay);
}
#endif
