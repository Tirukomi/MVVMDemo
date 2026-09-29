// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
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
	// Close, over-the-shoulder framing: the hero sits left of centre and the HUD frames the right.
	CameraBoom->TargetArmLength = 320.f;
	CameraBoom->SocketOffset = FVector(0.f, 70.f, 55.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Engine mannequin from the Third Person content pack (Content/Characters/Mannequins).
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HeroMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (HeroMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(HeroMesh.Object);
	}
	// The dark suit is applied in BeginPlay by soft path: it is a generated asset (Scripts/CreateArenaMap.py), and a
	// constructor reference would root it in the editor so the script could not rebuild it.
	static ConstructorHelpers::FClassFinder<UAnimInstance> HeroAnim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (HeroAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(HeroAnim.Class);
	}

	RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(GetCapsuleComponent());
	RimLight->SetRelativeLocation(FVector(-110.f, 0.f, 120.f));
	RimLight->SetIntensityUnits(ELightUnits::Candelas);
	RimLight->SetIntensity(40.f);
	RimLight->SetLightColor(FLinearColor(0.55f, 0.7f, 1.f));
	RimLight->SetAttenuationRadius(320.f);
	RimLight->SetCastShadows(false);
	RimLight->SetVolumetricScatteringIntensity(0.f);

	Health = CreateDefaultSubobject<UHealthComponent>(TEXT("Health"));
	Gadgets = CreateDefaultSubobject<UGadgetComponent>(TEXT("Gadgets"));
	Combo = CreateDefaultSubobject<UComboComponent>(TEXT("Combo"));
	Detective = CreateDefaultSubobject<UDetectiveComponent>(TEXT("Detective"));
	DetectiveVision = CreateDefaultSubobject<UDetectiveVisionComponent>(TEXT("DetectiveVision"));
}

void AGothamCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Suit = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Environment/M_Suit.M_Suit"))).LoadSynchronous())
	{
		for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
		{
			GetMesh()->SetMaterial(i, Suit);
		}
	}
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
