// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Templates/Function.h"

/**
 * A scripted scenario: a list of steps run one after another, frame by frame. The dev aids, the perf harness and the
 * menu input test are all scripts for it, so "do X, wait, check Y" lives in one place.
 *
 *     TSharedRef<FMvsScript> Script = MakeShared<FMvsScript>();
 *     Script->At(1.f).Do([] { OpenPause(); })
 *            .Wait(0.5f).Expect([] { return IsPauseOpen(); }, TEXT("pause opens"))
 *            .Screenshot(TEXT("mvs_pause")).Quit();
 *     Script->Start();
 *
 * Time is real time from the core ticker, not game time: the pause menu and the wheel's slow motion stop or slow the
 * world, and scripts must keep running. Timing rules (unit-tested with a fake clock, see Tests/ScriptTests.cpp):
 * - Do / Expect / Screenshot / Quit take no time; several in a row run in the same frame.
 * - At(T) waits until T seconds after the script started (the first frame's delta counts).
 * - Wait(S) and Sample(S) start counting on the frame after they begin, and end on the frame their own time reaches S;
 *   the steps after them run in that same frame. Sample calls its function on each of those frames.
 * - WaitFrames(N) ends on the Nth frame after the one it begins in.
 * - WaitUntil(P, S, Rule) checks P on the frame it begins and every frame after; it passes as soon as P holds and
 *   fails Rule if S seconds of its own time pass first. Either way the script goes on in that frame.
 */

/** One check's outcome. A known bug is expected to fail until it is fixed; when it starts passing, that is reported
 *  as a failure too, so the check gets promoted to a normal rule instead of staying marked. */
enum class EMvsCheck : uint8
{
	Passed,
	Failed,
	KnownBug,
	KnownBugFixed,
};
class MVVMSAMPLE_API FMvsScript : public TSharedFromThis<FMvsScript>
{
public:
	using FAction = TFunction<void()>;
	using FFrame = TFunction<void(float StepSeconds, float DeltaSeconds)>;
	/** Where check results go; by default "PASS: rule", "FAIL: rule", "KNOWN BUG: rule" or "KNOWN BUG FIXED: rule" in
	 *  LogMvsScript (see ResultLabel). */
	using FReporter = TFunction<void(EMvsCheck Result, const FString& Rule)>;

	FMvsScript& Wait(float Seconds);
	FMvsScript& At(float SecondsFromStart);
	FMvsScript& Do(FAction Action);
	FMvsScript& Expect(TFunction<bool()> Predicate, const FString& Rule);
	FMvsScript& WaitUntil(TFunction<bool()> Predicate, float TimeoutSeconds, const FString& Rule);
	/** Waits N whole frames (counting from the next one), e.g. for layout to catch up after a change that moves it. */
	FMvsScript& WaitFrames(int32 Frames);
	FMvsScript& Sample(float Seconds, FFrame EachFrame);
	FMvsScript& Screenshot(const FString& Name);
	FMvsScript& Quit();

	void SetReporter(FReporter InReporter) { Reporter = MoveTemp(InReporter); }
	/** Records one result (for checks inside a Do). */
	void Check(bool bPassed, const FString& Rule);
	/**
	 * Records a check that is known to fail today (bCorrect is what the fixed behaviour would give). An intermittent bug
	 * sometimes behaves correctly, so it is always recorded as known (with what this run saw) and never reported as
	 * fixed by chance; the fix removes the flag.
	 */
	void CheckKnownBug(bool bCorrect, const FString& Rule, bool bIntermittent = false);

	static const TCHAR* ResultLabel(EMvsCheck Result);

	/** Runs on the core ticker until the last step; the ticker keeps the script alive until then. */
	void Start();
	/** One frame. Returns true while steps remain. Start calls this every frame; tests call it directly. */
	bool Advance(float DeltaSeconds);

	bool IsFinished() const { return !Steps.IsValidIndex(Next); }
	double GetTime() const { return Time; }
	int32 GetPassed() const { return Passed; }
	int32 GetFailed() const { return Failed; }
	int32 GetKnownBugs() const { return KnownBugs; }

private:
	enum class EKind : uint8 { Do, At, Wait, Sample, WaitUntil, Frames };
	struct FStep
	{
		EKind Kind = EKind::Do;
		float Seconds = 0.f;
		FAction Action;
		FFrame EachFrame;
		TFunction<bool()> Predicate;
		FString Rule;
	};

	void Report(EMvsCheck Result, const FString& Rule);

	TArray<FStep> Steps;
	int32 Next = 0;
	double Time = 0.0;
	float StepTime = 0.f;
	/** Frame on which the current timed step began: it does not count that frame. */
	int64 Frame = 0;
	int64 StepStartFrame = -1;
	int32 Passed = 0;
	int32 Failed = 0;
	int32 KnownBugs = 0;
	FReporter Reporter;
	FTSTicker::FDelegateHandle Handle;
};
