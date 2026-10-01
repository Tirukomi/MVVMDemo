// Copyright IG. All Rights Reserved.

#include "Gameplay/MvsFeel.h"

#include "Gameplay/TimeScaleSubsystem.h"
#include "UI/Style/MvsMotion.h"

namespace MvsFeel
{
	void HitStop(const UObject* WorldContext, float Seconds)
	{
		UMvsTimeScaleSubsystem* TimeScale = UMvsTimeScaleSubsystem::Get(WorldContext);
		if (!TimeScale || MvsMotion::IsReduced(WorldContext))
		{
			return;
		}
		TimeScale->RequestFor(TEXT("HitStop"), HitStopDilation, Seconds);
	}
}
