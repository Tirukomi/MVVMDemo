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

FGothamScript& FGothamScript::WaitUntil(TFunction<bool()> Predicate, float TimeoutSeconds, const FString& Rule)
{
	FStep Step;
	Step.Kind = EKind::WaitUntil;
	Step.Seconds = TimeoutSeconds;
	Step.Predicate = MoveTemp(Predicate);
	Step.Rule = Rule;
	Steps.Add(MoveTemp(Step));
	return *this;
}

FGothamScript& FGothamScript::WaitFrames(int32 Frames)
{
	Steps.Add({ EKind::Frames, static_cast<float>(FMath::Max(Frames, 1)) });
	return *this;
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
	Report(bPassed ? EGothamCheck::Passed : EGothamCheck::Failed, Rule);
}

void FGothamScript::CheckKnownBug(bool bCorrect, const FString& Rule, bool bIntermittent)
{
	if (bIntermittent)
	{
		Report(EGothamCheck::KnownBug, FString::Printf(TEXT("%s [intermittent; this run: %s]"), *Rule, bCorrect ? TEXT("fine") : TEXT("seen")));
		return;
	}
	Report(bCorrect ? EGothamCheck::KnownBugFixed : EGothamCheck::KnownBug, Rule);
}

const TCHAR* FGothamScript::ResultLabel(EGothamCheck Result)
{
	switch (Result)
	{
	case EGothamCheck::Passed:        return TEXT("PASS");
	case EGothamCheck::Failed:        return TEXT("FAIL");
	case EGothamCheck::KnownBug:      return TEXT("KNOWN BUG");
	default:                          return TEXT("KNOWN BUG FIXED (promote to a rule)");
	}
}

void FGothamScript::Report(EGothamCheck Result, const FString& Rule)
{
	switch (Result)
	{
	case EGothamCheck::Passed:   ++Passed; break;
	case EGothamCheck::KnownBug: ++KnownBugs; break;
	default:                     ++Failed; break;
	}
	if (Reporter)
	{
		Reporter(Result, Rule);
	}
	else
	{
		UE_LOG(LogGothamScript, Display, TEXT("%s: %s"), ResultLabel(Result), *Rule);
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
		case EKind::Frames:
			if (StepStartFrame < 0)
			{
				StepStartFrame = Frame;
				return true;
			}
			if (Frame - StepStartFrame < static_cast<int64>(Step.Seconds))
			{
				return true;
			}
			StepStartFrame = -1;
			++Next;
			continue;
		case EKind::WaitUntil:
		{
			const bool bFirstFrame = StepStartFrame < 0;
			if (bFirstFrame)
			{
				StepStartFrame = Frame;
				StepTime = 0.f;
			}
			else
			{
				StepTime += DeltaSeconds;
			}
			const bool bMet = Step.Predicate && Step.Predicate();
			if (!bMet && StepTime + KINDA_SMALL_NUMBER < Step.Seconds)
			{
				return true;
			}
			const FString Rule = Step.Rule;
			StepStartFrame = -1;
			++Next;
			Check(bMet, Rule);
			continue;
		}
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
