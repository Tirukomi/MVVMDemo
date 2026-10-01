// Copyright IG. All Rights Reserved.

#include "Core/MvsScript.h"

#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsScript, Log, All);

FMvsScript& FMvsScript::Wait(float Seconds)
{
	Steps.Add({ EKind::Wait, Seconds });
	return *this;
}

FMvsScript& FMvsScript::At(float SecondsFromStart)
{
	Steps.Add({ EKind::At, SecondsFromStart });
	return *this;
}

FMvsScript& FMvsScript::Do(FAction Action)
{
	FStep Step;
	Step.Kind = EKind::Do;
	Step.Action = MoveTemp(Action);
	Steps.Add(MoveTemp(Step));
	return *this;
}

FMvsScript& FMvsScript::Expect(TFunction<bool()> Predicate, const FString& Rule)
{
	// The script outlives its steps, so the raw pointer is safe.
	return Do([this, Predicate = MoveTemp(Predicate), Rule]() { Check(Predicate(), Rule); });
}

FMvsScript& FMvsScript::WaitUntil(TFunction<bool()> Predicate, float TimeoutSeconds, const FString& Rule)
{
	FStep Step;
	Step.Kind = EKind::WaitUntil;
	Step.Seconds = TimeoutSeconds;
	Step.Predicate = MoveTemp(Predicate);
	Step.Rule = Rule;
	Steps.Add(MoveTemp(Step));
	return *this;
}

FMvsScript& FMvsScript::WaitFrames(int32 Frames)
{
	Steps.Add({ EKind::Frames, static_cast<float>(FMath::Max(Frames, 1)) });
	return *this;
}

FMvsScript& FMvsScript::Sample(float Seconds, FFrame EachFrame)
{
	FStep Step;
	Step.Kind = EKind::Sample;
	Step.Seconds = Seconds;
	Step.EachFrame = MoveTemp(EachFrame);
	Steps.Add(MoveTemp(Step));
	return *this;
}

FMvsScript& FMvsScript::Screenshot(const FString& Name)
{
	return Do([Name]() { FScreenshotRequest::RequestScreenshot(Name, true, false); });
}

FMvsScript& FMvsScript::Quit()
{
	return Do([]() { FPlatformMisc::RequestExit(false); });
}

void FMvsScript::Check(bool bPassed, const FString& Rule)
{
	Report(bPassed ? EMvsCheck::Passed : EMvsCheck::Failed, Rule);
}

void FMvsScript::CheckKnownBug(bool bCorrect, const FString& Rule, bool bIntermittent)
{
	if (bIntermittent)
	{
		Report(EMvsCheck::KnownBug, FString::Printf(TEXT("%s [intermittent; this run: %s]"), *Rule, bCorrect ? TEXT("fine") : TEXT("seen")));
		return;
	}
	Report(bCorrect ? EMvsCheck::KnownBugFixed : EMvsCheck::KnownBug, Rule);
}

const TCHAR* FMvsScript::ResultLabel(EMvsCheck Result)
{
	switch (Result)
	{
	case EMvsCheck::Passed:        return TEXT("PASS");
	case EMvsCheck::Failed:        return TEXT("FAIL");
	case EMvsCheck::KnownBug:      return TEXT("KNOWN BUG");
	default:                          return TEXT("KNOWN BUG FIXED (promote to a rule)");
	}
}

void FMvsScript::Report(EMvsCheck Result, const FString& Rule)
{
	switch (Result)
	{
	case EMvsCheck::Passed:   ++Passed; break;
	case EMvsCheck::KnownBug: ++KnownBugs; break;
	default:                     ++Failed; break;
	}
	if (Reporter)
	{
		Reporter(Result, Rule);
	}
	else
	{
		UE_LOG(LogMvsScript, Display, TEXT("%s: %s"), ResultLabel(Result), *Rule);
	}
}

void FMvsScript::Start()
{
	const TSharedRef<FMvsScript> Self = AsShared();
	Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Self](float DeltaSeconds) { return Self->Advance(DeltaSeconds); }));
}

bool FMvsScript::Advance(float DeltaSeconds)
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
