// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/DetectiveVisionComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "UObject/ConstructorHelpers.h"

AGothamCharacter::AGothamCharacter()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Placeholder body so the character is visible without any imported art.
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetCapsuleComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetRelativeLocation(FVector(0.f, 0.f, -3.f));
	Body->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.7f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Body->SetStaticMesh(Cylinder.Object);
	}

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Gadgets = CreateDefaultSubobject<UGadgetComponent>(TEXT("Gadgets"));
	Combo = CreateDefaultSubobject<UComboComponent>(TEXT("Combo"));
	Detective = CreateDefaultSubobject<UDetectiveComponent>(TEXT("Detective"));
	DetectiveVision = CreateDefaultSubobject<UDetectiveVisionComponent>(TEXT("DetectiveVision"));
}

void AGothamCharacter::MoveInput(const FVector2D& Axis)
{
	if (Controller)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void AGothamCharacter::LookInput(const FVector2D& Axis)
{
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AGothamCharacter::Attack()
{
	Combo->RegisterHit();
}

void AGothamCharacter::UseGadget(int32 SlotIndex)
{
	Gadgets->UseGadget(SlotIndex);
}

void AGothamCharacter::ToggleDetective()
{
	Detective->ToggleDetective();
}

void AGothamCharacter::ScanClue()
{
	Detective->TryScan();
}

void AGothamCharacter::DebugDamage()
{
	Health->ApplyDamage(FMath::FRandRange(8.f, 20.f));
}

void AGothamCharacter::DebugHeal()
{
	Health->Heal(15.f);
}
