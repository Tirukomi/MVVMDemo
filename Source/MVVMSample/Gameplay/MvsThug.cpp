// Copyright IG. All Rights Reserved.

#include "Gameplay/MvsThug.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/MvsCharacter.h"
#include "Gameplay/ForensicTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/MvsFeel.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** A UI sandbox should not loop the player into a death screen: thugs pull their punches near the end. */
	constexpr float MercyHealth = 15.f;
	constexpr float TurnSpeedDegPerSec = 360.f;
}

AMvsThug::AMvsThug()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	// No controller drives it, but knockback and gravity still need the movement component to simulate.
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->GravityScale = 1.f;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ThugMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (ThugMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(ThugMesh.Object);
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> Anim(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (Anim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(Anim.Class);
	}
	// Forensic Mode draws stencil 3 as hostile (see Scripts/CreateForensicAssets.py).
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(static_cast<int32>(EMvsStencil::Hostile));
}

void AMvsThug::BeginPlay()
{
	Super::BeginPlay();
	// No material override: thugs keep the mannequin's own textured materials (MI_Manny_01/02_New), which read
	// clearly apart from the hero's dark suit. A generated material here would need the skeletal-mesh usage flag,
	// or the engine silently substitutes its default material.
	if (UMvsThreatSubsystem* Threats = GetWorld()->GetSubsystem<UMvsThreatSubsystem>())
	{
		Threats->Register(this);
	}
}

void AMvsThug::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMvsThreatSubsystem* Threats = GetWorld()->GetSubsystem<UMvsThreatSubsystem>())
	{
		Threats->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AMvsThug::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Keep facing the player unless stunned (a countered thug reels away).
	const APawn* Player = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Player || Brain.State == EMvsThugState::Stunned)
	{
		return;
	}
	const FVector ToPlayer = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!ToPlayer.IsNearlyZero())
	{
		const FRotator Target = ToPlayer.Rotation();
		SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), FRotator(0.f, Target.Yaw, 0.f), DeltaSeconds, TurnSpeedDegPerSec));
	}
}

FVector AMvsThug::GetPromptLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 45.f);
}

void AMvsThug::Strike(APawn* Target)
{
	if (!Target)
	{
		return;
	}
	// A short lunge sells the strike whether or not it lands.
	const FVector ToTarget = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	LaunchCharacter(ToTarget * 420.f, true, false);

	if (FVector::Dist2D(Target->GetActorLocation(), GetActorLocation()) > StrikeRange)
	{
		return; // whiffed: the player stepped out of reach
	}
	AMvsCharacter* Hero = Cast<AMvsCharacter>(Target);
	UHealthComponent* Health = Hero ? Hero->GetHealthComponent() : nullptr;
	if (Health && Health->GetHealth() > MercyHealth)
	{
		Health->ApplyDamage(FMath::Min(StrikeDamage, Health->GetHealth() - MercyHealth));
		Hero->AddCameraTrauma(0.55f);
		MvsFeel::HitStop(this, MvsFeel::HitStopSeconds);
	}
}

void AMvsThug::OnCountered(const APawn* By)
{
	const FVector Away = By ? (GetActorLocation() - By->GetActorLocation()).GetSafeNormal2D() : -GetActorForwardVector();
	LaunchCharacter(Away * 650.f + FVector(0.f, 0.f, 260.f), true, true);
}
