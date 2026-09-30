// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamGameMode.h"

#include "Core/GothamCharacter.h"
#include "Core/GothamPlayerController.h"
#include "Engine/World.h"
#include "Gameplay/GothamThug.h"
#include "Misc/CommandLine.h"

AGothamGameMode::AGothamGameMode()
{
	DefaultPawnClass = AGothamCharacter::StaticClass();
	PlayerControllerClass = AGothamPlayerController::StaticClass();

	// Around the player start (0, -250, facing +Y): two ahead, one off to the right, one behind, so both the
	// counter prompts and the off-screen arrows have something to show. Pairs are (spot, fallback, fallback).
	ThugSpots = {
		FVector2D(220.f, 330.f), FVector2D(160.f, 380.f), FVector2D(280.f, 280.f),
		FVector2D(-320.f, 240.f), FVector2D(-260.f, 320.f), FVector2D(-380.f, 160.f),
		FVector2D(640.f, -300.f), FVector2D(560.f, -420.f), FVector2D(700.f, -180.f),
		FVector2D(-120.f, -780.f), FVector2D(140.f, -820.f), FVector2D(-320.f, -720.f),
	};
}

void AGothamGameMode::StartPlay()
{
	Super::StartPlay();
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("GothamNoThugs")))
	{
		return;
	}
#endif
	SpawnThugs();
}

void AGothamGameMode::SpawnThugs()
{
	UWorld* World = GetWorld();
	constexpr int32 SpotsPerThug = 3;
	constexpr float RoofTopZ = 0.f;
	for (int32 Base = 0; Base + SpotsPerThug <= ThugSpots.Num(); Base += SpotsPerThug)
	{
		for (int32 i = 0; i < SpotsPerThug; ++i)
		{
			const FVector2D Spot = ThugSpots[Base + i];
			FHitResult Hit;
			FCollisionQueryParams Params(TEXT("ThugSpawn"));
			if (!World->LineTraceSingleByChannel(Hit, FVector(Spot.X, Spot.Y, 800.f), FVector(Spot.X, Spot.Y, -200.f), ECC_Visibility, Params)
				|| Hit.ImpactPoint.Z > RoofTopZ + 20.f)
			{
				continue; // missed the roof, or landed on a vent: try the fallback
			}
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
			if (World->SpawnActor<AGothamThug>(AGothamThug::StaticClass(), Hit.ImpactPoint + FVector(0.f, 0.f, 98.f), FRotator(0.f, -90.f, 0.f), SpawnParams))
			{
				break;
			}
		}
	}
}
