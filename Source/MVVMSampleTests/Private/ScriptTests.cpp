// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Core/GothamScript.h"

#if WITH_DEV_AUTOMATION_TESTS

// The scripted-scenario runner's timing, driven by hand (a fake clock): every script in the game depends on it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamScriptTimingTest, "Gotham.Script.Timing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamScriptTimingTest::RunTest(const FString& Parameters)
{
	TArray<FString> Log;
	int32 Frame = 0;
	auto Mark = [&Log, &Frame](const TCHAR* What) { return [&Log, &Frame, What]() { Log.Add(FString::Printf(TEXT("%s@%d"), What, Frame)); }; };
	TArray<float> SampleTimes;

	TSharedRef<FGothamScript> Script = MakeShared<FGothamScript>();
	Script->Do(Mark(TEXT("start")))
		.At(0.3f).Do(Mark(TEXT("at0.3")))
		.Wait(0.2f).Do(Mark(TEXT("waited")))
		.Sample(0.3f, [&SampleTimes](float StepSeconds, float) { SampleTimes.Add(StepSeconds); })
		.Do(Mark(TEXT("sampled"))).Do(Mark(TEXT("same frame")));

	// Frames of 0.1 s. A mark records how many frames had completed before it ran (the first frame is 0, t = 0.1).
	while (Script->Advance(0.1f) && Frame < 100)
	{
		++Frame;
	}

	const TArray<FString> Expected = {
		TEXT("start@0"),       // zero-time steps run on the first frame
		TEXT("at0.3@2"),       // At counts from the start, the first frame included: t reaches 0.3 on frame 2
		TEXT("waited@4"),      // Wait begins on frame 2, counts frames 3 and 4, and ends on frame 4
		TEXT("sampled@7"),     // Sample begins on frame 4, samples frames 5, 6 and 7, ends on frame 7
		TEXT("same frame@7"),  // steps after a timed step run in the frame it ends
	};
	TestEqual("step order and frames", Log, Expected);
	TestEqual("Sample runs once per counted frame", SampleTimes.Num(), 3);
	if (SampleTimes.Num() == 3)
	{
		TestTrue("Sample reports its own elapsed time", FMath::IsNearlyEqual(SampleTimes[0], 0.1f) && FMath::IsNearlyEqual(SampleTimes[2], 0.3f));
	}
	TestTrue("finished", Script->IsFinished());
	TestFalse("advancing a finished script does nothing", Script->Advance(0.1f));

	// Expect / Check count results and go to the reporter.
	TSharedRef<FGothamScript> Checks = MakeShared<FGothamScript>();
	TArray<FString> Reported;
	Checks->SetReporter([&Reported](EGothamCheck Result, const FString& Rule) { Reported.Add(FString::Printf(TEXT("%s: %s"), FGothamScript::ResultLabel(Result), *Rule)); });
	Checks->Expect([] { return true; }, TEXT("yes")).Expect([] { return false; }, TEXT("no"));
	Checks->Do([&Checks] { Checks->CheckKnownBug(false, TEXT("still broken")); Checks->CheckKnownBug(true, TEXT("now works")); Checks->CheckKnownBug(true, TEXT("flaky"), true); });
	Checks->Advance(0.f);
	TestEqual("passed", Checks->GetPassed(), 1);
	TestEqual("failed (a fixed known bug counts: it must be promoted)", Checks->GetFailed(), 2);
	TestEqual("known bugs (an intermittent one never counts as fixed)", Checks->GetKnownBugs(), 2);
	TestEqual("reported word for word", Reported, TArray<FString>{ TEXT("PASS: yes"), TEXT("FAIL: no"),
		TEXT("KNOWN BUG: still broken"), TEXT("KNOWN BUG FIXED (promote to a rule): now works"),
		TEXT("KNOWN BUG: flaky [intermittent; this run: fine]") });

	// WaitUntil: passes the frame its condition holds (the frame it begins included), fails on its own timeout, and
	// the script carries on either way.
	TSharedRef<FGothamScript> Waits = MakeShared<FGothamScript>();
	bool bReady = false;
	int32 WaitFrame = 0;
	TArray<FString> WaitLog;
	Waits->SetReporter([&WaitLog, &WaitFrame](EGothamCheck Result, const FString& Rule) { WaitLog.Add(FString::Printf(TEXT("%s %s@%d"), FGothamScript::ResultLabel(Result), *Rule, WaitFrame)); });
	Waits->WaitUntil([] { return true; }, 1.f, TEXT("already true"))
		.WaitUntil([&bReady] { return bReady; }, 1.f, TEXT("becomes true"))
		.WaitUntil([] { return false; }, 0.25f, TEXT("never true"))
		.Do([&WaitLog, &WaitFrame] { WaitLog.Add(FString::Printf(TEXT("after@%d"), WaitFrame)); });
	for (WaitFrame = 0; WaitFrame < 20 && Waits->Advance(0.1f); ++WaitFrame)
	{
		bReady = WaitFrame >= 2; // true from the frame after frame 2's check
	}
	TestEqual("WaitUntil timing", WaitLog, TArray<FString>{
		TEXT("PASS already true@0"),   // true on the frame it begins
		TEXT("PASS becomes true@3"),   // checked every frame; bReady is set after frame 2 ran
		TEXT("FAIL never true@6"),     // begins on frame 3, times out once its own 0.25 s have passed (frames 4, 5, 6)
		TEXT("after@6") });            // the script goes on in the same frame

	// WaitFrames counts whole frames from the next one.
	TSharedRef<FGothamScript> Frames = MakeShared<FGothamScript>();
	int32 FrameCount = 0;
	int32 DoneAt = -1;
	Frames->WaitFrames(2).Do([&DoneAt, &FrameCount] { DoneAt = FrameCount; });
	for (FrameCount = 0; FrameCount < 10 && Frames->Advance(0.1f); ++FrameCount) {}
	TestEqual("WaitFrames(2) begins on frame 0 and ends on frame 2", DoneAt, 2);

	// A step may add steps while it runs.
	TSharedRef<FGothamScript> Growing = MakeShared<FGothamScript>();
	bool bRanAdded = false;
	FGothamScript* Raw = &Growing.Get();
	Growing->Do([Raw, &bRanAdded] { for (int32 i = 0; i < 64; ++i) { Raw->Do([&bRanAdded] { bRanAdded = true; }); } });
	Growing->Advance(0.f);
	TestTrue("steps added by a step run too", bRanAdded && Growing->IsFinished());
	return true;
}

#endif
