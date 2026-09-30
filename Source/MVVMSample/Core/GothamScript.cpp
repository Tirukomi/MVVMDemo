// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamScript.h"

#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamScript, Log, All);

FGothamScript& FGothamScript::Wait(float Seconds)
{
	Steps.Add({ EKind::Wait, Seconds });
	return *this;
}

FGothamScript& FGothamScript::At(float SecondsFromStart)
{
	Steps.Add({ EKind::At, SecondsFromStart });
	return *this;
}

FGothamScript& FGothamScript::Do(FAction Action)
{
	FStep Step;
	Step.Kind = EKind::Do;
	Step.Action = MoveTemp(Action);
	Steps.Add(MoveTemp(Step));
	return *this;
}

FGothamScript& FGothamScript::Expect(TFunction<bool()> Predicate, const FString& Rule)
{
	// The script outlives its steps, so the raw pointer is safe.
	return Do([this, Predicate = MoveTemp(Predicate), Rule]() { Check(Predicate(), Rule); });
}

FGothamScript& FGothamScript::Sample(float Seconds, FFrame EachFrame)
{
	FStep Step;
	Step.Kind = EKind::Sample;
	Step.Seconds = Seconds;
	Step.EachFrame = MoveTemp(EachFrame);
	Steps.Add(MoveTemp(Step));
	return *this;
}

FGothamScript& FGothamScript::Screenshot(const FString& Name)
{
	return Do([Name]() { FScreenshotRequest::RequestScreenshot(Name, true, false); });
}

FGothamScript& FGothamScript::Quit()
{
	return Do([]() { FPlatformMisc::RequestExit(false); });
}

void FGothamScript::Check(bool bPassed, const FString& Rule)
{
	(bPassed ? Passed : Failed)++;
	if (Reporter)
	{
		Reporter(bPassed, Rule);
	}
	else
	{
		UE_LOG(LogGothamScript, Display, TEXT("%s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Rule);
	}
}

void FGothamScript::Start()
{
	const TSharedRef<FGothamScript> Self = AsShared();
	Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self](float DeltaSeconds) { return Self->Advance(DeltaSeconds); }));
}

bool FGothamScript::Advance(float DeltaSeconds)
{
	++Frame;
	Time += DeltaSeconds;
	while (Steps.IsValidIndex(Next))
	{
		FStep& Step = Steps[Next];
		switch (Step.Kind)
		{
		case EKind::Do:
		{
			// Moved out first: an action may add steps, which can reallocate the array under a reference.
			const FAction Action = MoveTemp(Step.Action);
			++Next;
			if (Action)
			{
				Action();
			}
			continue;
		}
		case EKind::At:
			if (Time + KINDA_SMALL_NUMBER < Step.Seconds)
			{
				return true;
			}
			++Next;
			continue;
		case EKind::Wait:
		case EKind::Sample:
			if (StepStartFrame < 0)
			{
				// Begins now; its time starts with the next frame.
				StepStartFrame = Frame;
				StepTime = 0.f;
				return true;
			}
			StepTime += DeltaSeconds;
			if (Step.Kind == EKind::Sample && Step.EachFrame)
			{
				Step.EachFrame(StepTime, DeltaSeconds);
			}
			if (StepTime + KINDA_SMALL_NUMBER < Step.Seconds)
			{
				return true;
			}
			StepStartFrame = -1;
			++Next;
			continue;
		}
	}
	return false;
}
