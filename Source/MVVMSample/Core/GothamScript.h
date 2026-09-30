// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Templates/Function.h"

/**
 * A scripted scenario: a list of steps run one after another, frame by frame. The dev aids, the perf harness and the
 * menu input test are all scripts for it, so "do X, wait, check Y" lives in one place.
 *
 *     TSharedRef<FGothamScript> Script = MakeShared<FGothamScript>();
 *     Script->At(1.f).Do([] { OpenPause(); })
 *            .Wait(0.5f).Expect([] { return IsPauseOpen(); }, TEXT("pause opens"))
 *            .Screenshot(TEXT("gotham_pause")).Quit();
 *     Script->Start();
 *
 * Time is real time from the core ticker, not game time: the pause menu and the wheel's slow motion stop or slow the
 * world, and scripts must keep running. Timing rules (unit-tested with a fake clock, see Tests/ScriptTests.cpp):
 * - Do / Expect / Screenshot / Quit take no time; several in a row run in the same frame.
 * - At(T) waits until T seconds after the script started (the first frame's delta counts).
 * - Wait(S) and Sample(S) start counting on the frame after they begin, and end on the frame their own time reaches S;
 *   the steps after them run in that same frame. Sample calls its function on each of those frames.
 */
class MVVMSAMPLE_API FGothamScript : public TSharedFromThis<FGothamScript>
{
public:
	using FAction = TFunction<void()>;
	using FFrame = TFunction<void(float StepSeconds, float DeltaSeconds)>;
	/** Where Expect / Check results go; by default "PASS: rule" / "FAIL: rule" in LogGothamScript. */
	using FReporter = TFunction<void(bool bPassed, const FString& Rule)>;

	FGothamScript& Wait(float Seconds);
	FGothamScript& At(float SecondsFromStart);
	FGothamScript& Do(FAction Action);
	FGothamScript& Expect(TFunction<bool()> Predicate, const FString& Rule);
	FGothamScript& Sample(float Seconds, FFrame EachFrame);
	FGothamScript& Screenshot(const FString& Name);
	FGothamScript& Quit();

	void SetReporter(FReporter InReporter) { Reporter = MoveTemp(InReporter); }
	/** Records one result (for checks inside a Do). */
	void Check(bool bPassed, const FString& Rule);

	/** Runs on the core ticker until the last step; the ticker keeps the script alive until then. */
	void Start();
	/** One frame. Returns true while steps remain. Start calls this every frame; tests call it directly. */
	bool Advance(float DeltaSeconds);

	bool IsFinished() const { return !Steps.IsValidIndex(Next); }
	double GetTime() const { return Time; }
	int32 GetPassed() const { return Passed; }
	int32 GetFailed() const { return Failed; }

private:
	enum class EKind : uint8 { Do, At, Wait, Sample };
	struct FStep
	{
		EKind Kind = EKind::Do;
		float Seconds = 0.f;
		FAction Action;
		FFrame EachFrame;
	};

	TArray<FStep> Steps;
	int32 Next = 0;
	double Time = 0.0;
	float StepTime = 0.f;
	/** Frame on which the current timed step began: it does not count that frame. */
	int64 Frame = 0;
	int64 StepStartFrame = -1;
	int32 Passed = 0;
	int32 Failed = 0;
	FReporter Reporter;
	FTSTicker::FDelegateHandle Handle;
};
