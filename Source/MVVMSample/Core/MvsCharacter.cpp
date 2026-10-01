// Copyright IG. All Rights Reserved.

#include "Core/MvsCharacter.h"

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
#include "Gameplay/ForensicComponent.h"
#include "Gameplay/ForensicVisionComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/MvsFeel.h"
#include "Gameplay/MvsThug.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UI/Style/MvsMotion.h"
#include "UObject/ConstructorHelpers.h"

AMvsCharacter::AMvsCharacter()
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
	Forensic = CreateDefaultSubobject<UForensicComponent>(TEXT("Forensic"));
	ForensicVision = CreateDefaultSubobject<UForensicVisionComponent>(TEXT("ForensicVision"));
}

void AMvsCharacter::BeginPlay()
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

void AMvsCharacter::MoveInput(const FVector2D& Axis)
{
	if (Controller)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
	}
}

void AMvsCharacter::LookInput(const FVector2D& Axis)
{
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AMvsCharacter::Attack()
{
	Combo->RegisterHit();
}

void AMvsCharacter::UseGadget(int32 SlotIndex)
{
	Gadgets->UseGadget(SlotIndex);
}

void AMvsCharacter::ToggleForensic()
{
	Forensic->ToggleForensic();
}

void AMvsCharacter::ScanClue()
{
	Forensic->TryScan();
}

bool AMvsCharacter::Counter()
{
	UMvsThreatSubsystem* Threats = GetWorld()->GetSubsystem<UMvsThreatSubsystem>();
	if (!Threats || !Threats->TryCounter(this))
	{
		return false;
	}
	// A counter is a hit: it builds the combo, freezes the moment and jolts the camera a little.
	Combo->RegisterHit();
	MvsFeel::HitStop(this, MvsFeel::HitStopSeconds);
	AddCameraTrauma(0.3f);
	return true;
}

void AMvsCharacter::AddCameraTrauma(float Amount)
{
	if (MvsMotion::IsReduced(this))
	{
		return;
	}
	Trauma.Add(Amount);
	bShaking = true;
}

void AMvsCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bShaking)
	{
		return;
	}
	// Real time, so a hit-stop does not freeze the shake that sells it.
	const float RealDelta = FApp::GetDeltaTime();
	ShakeTime += RealDelta;
	bShaking = Trauma.Advance(RealDelta);
	const float Shake = Trauma.GetShake();
	constexpr float MaxAngle = 2.2f;
	auto Noise = [this](float Seed) { return FMath::PerlinNoise1D(ShakeTime * 22.f + Seed); };
	FollowCamera->SetRelativeRotation(bShaking
		? FRotator(Noise(0.f) * MaxAngle * Shake, Noise(31.7f) * MaxAngle * Shake, Noise(67.3f) * MaxAngle * 0.5f * Shake)
		: FRotator::ZeroRotator);
}

void AMvsCharacter::DebugDamage()
{
	Health->ApplyDamage(FMath::FRandRange(8.f, 20.f));
	AddCameraTrauma(0.4f);
}

void AMvsCharacter::DebugHeal()
{
	Health->Heal(15.f);
}
