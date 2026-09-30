// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GothamFeel.h"

#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Style/GothamMotion.h"

namespace GothamFeel
{
	namespace
	{
		FTSTicker::FDelegateHandle RestoreHandle;
		float RemainingSeconds = 0.f;
	}

	void HitStop(const UObject* WorldContext, float Seconds)
	{
		UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World || GothamMotion::IsReduced(WorldContext))
		{
			return;
		}
		const float Current = UGameplayStatics::GetGlobalTimeDilation(World);
		const bool bOurs = RestoreHandle.IsValid();
		if (!bOurs && !FMath::IsNearlyEqual(Current, 1.f))
		{
			return; // someone else (the gadget wheel) is bending time; do not fight it
		}
		RemainingSeconds = FMath::Max(RemainingSeconds, Seconds);
		UGameplayStatics::SetGlobalTimeDilation(World, HitStopDilation);
		if (bOurs)
		{
			return;
		}
		const TWeakObjectPtr<UWorld> WeakWorld(World);
		RestoreHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakWorld](float RealDelta)
		{
			RemainingSeconds -= RealDelta;
			if (RemainingSeconds > 0.f && WeakWorld.IsValid())
			{
				return true;
			}
			if (UWorld* W = WeakWorld.Get())
			{
				UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
			}
			RemainingSeconds = 0.f;
			RestoreHandle.Reset();
			return false;
		}));
	}
}
