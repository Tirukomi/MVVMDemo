// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GothamFeel.h"

#include "Gameplay/TimeScaleSubsystem.h"
#include "UI/Style/GothamMotion.h"

namespace GothamFeel
{
	void HitStop(const UObject* WorldContext, float Seconds)
	{
		UGothamTimeScaleSubsystem* TimeScale = UGothamTimeScaleSubsystem::Get(WorldContext);
		if (!TimeScale || GothamMotion::IsReduced(WorldContext))
		{
			return;
		}
		TimeScale->RequestFor(TEXT("HitStop"), HitStopDilation, Seconds);
	}
}
