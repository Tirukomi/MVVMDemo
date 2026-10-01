// Copyright IG. All Rights Reserved.

#include "Gameplay/TimeScaleSubsystem.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UMvsTimeScaleSubsystem* UMvsTimeScaleSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UMvsTimeScaleSubsystem>() : nullptr;
}

void UMvsTimeScaleSubsystem::Request(FName Source, float Scale)
{
	Requests.Add(Source, Scale);
	Expiry.Remove(Source);
	Apply();
}

void UMvsTimeScaleSubsystem::RequestFor(FName Source, float Scale, float RealSeconds)
{
	const double Until = FPlatformTime::Seconds() + RealSeconds;
	const double* Current = Expiry.Find(Source);
	Expiry.Add(Source, Current ? FMath::Max(*Current, Until) : Until);
	Requests.Add(Source, Scale);
	if (!TimedHandle.IsValid())
	{
		TimedHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UMvsTimeScaleSubsystem::TickTimed));
	}
	Apply();
}

void UMvsTimeScaleSubsystem::Clear(FName Source)
{
	Expiry.Remove(Source);
	if (Requests.Remove(Source) > 0)
	{
		Apply();
	}
}

float UMvsTimeScaleSubsystem::GetScale() const
{
	float Scale = 1.f;
	for (const TPair<FName, float>& Pair : Requests)
	{
		Scale = FMath::Min(Scale, Pair.Value);
	}
	return Scale;
}

void UMvsTimeScaleSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TimedHandle);
	TimedHandle.Reset();
	Super::Deinitialize();
}

void UMvsTimeScaleSubsystem::Apply()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, GetScale());
	}
}

bool UMvsTimeScaleSubsystem::TickTimed(float DeltaTime)
{
	const double Now = FPlatformTime::Seconds();
	TArray<FName> Expired;
	for (const TPair<FName, double>& Pair : Expiry)
	{
		if (Pair.Value <= Now)
		{
			Expired.Add(Pair.Key);
		}
	}
	for (const FName& Source : Expired)
	{
		Clear(Source);
	}
	if (Expiry.IsEmpty())
	{
		TimedHandle.Reset();
		return false;
	}
	return true;
}
