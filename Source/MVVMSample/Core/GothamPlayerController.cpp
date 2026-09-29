// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Core/GothamCharacter.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "UI/GothamUISettings.h"
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

	/** Swizzles a 1D key onto the Y axis so W/S drive forward/back on the 2D Move action. */
	void MapAxisKey(UInputMappingContext* Context, UInputAction* Action, const FKey& Key, bool bSwizzleToY, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Action, Key);
		if (bSwizzleToY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
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

	if (const TSubclassOf<UUserWidget> HudClass = GetDefault<UGothamUISettings>()->HudWidgetClass.LoadSynchronous())
	{
		HudWidget = CreateWidget<UUserWidget>(this, HudClass);
		HudWidget->AddToViewport();
		UE_LOG(LogGothamHud, Log, TEXT("HUD created: %s"), *GetNameSafe(HudClass));
	}
	else
	{
		UE_LOG(LogGothamHud, Warning, TEXT("No HUD widget class configured in Gotham UI settings"));
	}
	BindViewModelsToPawn();
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
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AGothamPlayerController::OnAttack);
		for (int32 i = 0; i < GadgetActions.Num(); ++i)
		{
			EIC->BindAction(GadgetActions[i], ETriggerEvent::Started, this, &AGothamPlayerController::OnGadget, i);
		}
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
	DebugDamageAction = MakeAction(this, TEXT("IA_DebugDamage"), EInputActionValueType::Boolean);
	DebugHealAction = MakeAction(this, TEXT("IA_DebugHeal"), EInputActionValueType::Boolean);

	// Move: WASD + left stick.
	MapAxisKey(GameplayContext, MoveAction, EKeys::W, true, false);
	MapAxisKey(GameplayContext, MoveAction, EKeys::S, true, true);
	MapAxisKey(GameplayContext, MoveAction, EKeys::D, false, false);
	MapAxisKey(GameplayContext, MoveAction, EKeys::A, false, true);
	GameplayContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Look: mouse + right stick.
	MapNegatedY(GameplayContext, LookAction, EKeys::Mouse2D);
	MapNegatedY(GameplayContext, LookAction, EKeys::Gamepad_Right2D);

	GameplayContext->MapKey(AttackAction, EKeys::LeftMouseButton);
	GameplayContext->MapKey(AttackAction, EKeys::Gamepad_FaceButton_Bottom);

	const FKey GadgetKeys[] = { EKeys::One, EKeys::Two, EKeys::Three };
	const FKey GadgetPadKeys[] = { EKeys::Gamepad_FaceButton_Left, EKeys::Gamepad_FaceButton_Top, EKeys::Gamepad_FaceButton_Right };
	for (int32 i = 0; i < 3; ++i)
	{
		UInputAction* Action = MakeAction(this, *FString::Printf(TEXT("IA_Gadget%d"), i + 1), EInputActionValueType::Boolean);
		GameplayContext->MapKey(Action, GadgetKeys[i]);
		GameplayContext->MapKey(Action, GadgetPadKeys[i]);
		GadgetActions.Add(Action);
	}

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
