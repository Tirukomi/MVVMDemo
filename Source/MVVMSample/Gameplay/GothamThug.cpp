// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GothamThug.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/GothamCharacter.h"
#include "Gameplay/DetectiveTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/GothamFeel.h"
#include "Gameplay/HealthComponent.h"
#include "Gameplay/ThreatSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** A UI sandbox should not loop the player into a death screen: thugs pull their punches near the end. */
	constexpr float MercyHealth = 15.f;
	constexpr float TurnSpeedDegPerSec = 360.f;
}

AGothamThug::AGothamThug()
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
	// Detective Mode draws stencil 3 as hostile (see Scripts/CreateDetectiveAssets.py).
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(static_cast<int32>(EGothamStencil::Hostile));
}

void AGothamThug::BeginPlay()
{
	Super::BeginPlay();
	// Dark metal rather than the hero's suit, so the silhouettes read apart. Soft path, as for the hero.
	if (UMaterialInterface* Look = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/Environment/M_DarkMetal.M_DarkMetal"))).LoadSynchronous())
	{
		for (int32 i = 0; i < GetMesh()->GetNumMaterials(); ++i)
		{
			GetMesh()->SetMaterial(i, Look);
		}
	}
	if (UGothamThreatSubsystem* Threats = GetWorld()->GetSubsystem<UGothamThreatSubsystem>())
	{
		Threats->Register(this);
	}
}

void AGothamThug::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGothamThreatSubsystem* Threats = GetWorld()->GetSubsystem<UGothamThreatSubsystem>())
	{
		Threats->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AGothamThug::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Keep facing the player unless stunned (a countered thug reels away).
	const APawn* Player = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
	if (!Player || Brain.State == EGothamThugState::Stunned)
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

FVector AGothamThug::GetPromptLocation() const
{
	return GetActorLocation() + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 45.f);
}

void AGothamThug::Strike(APawn* Target)
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
	AGothamCharacter* Hero = Cast<AGothamCharacter>(Target);
	UHealthComponent* Health = Hero ? Hero->GetHealthComponent() : nullptr;
	if (Health && Health->GetHealth() > MercyHealth)
	{
		Health->ApplyDamage(FMath::Min(StrikeDamage, Health->GetHealth() - MercyHealth));
		Hero->AddCameraTrauma(0.55f);
		GothamFeel::HitStop(this, GothamFeel::HitStopSeconds);
	}
}

void AGothamThug::OnCountered(const APawn* By)
{
	const FVector Away = By ? (GetActorLocation() - By->GetActorLocation()).GetSafeNormal2D() : -GetActorForwardVector();
	LaunchCharacter(Away * 650.f + FVector(0.f, 0.f, 260.f), true, true);
}
