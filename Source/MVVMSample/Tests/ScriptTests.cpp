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
	Checks->SetReporter([&Reported](bool bPassed, const FString& Rule) { Reported.Add(FString::Printf(TEXT("%s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Rule)); });
	Checks->Expect([] { return true; }, TEXT("yes")).Expect([] { return false; }, TEXT("no"));
	Checks->Advance(0.f);
	TestEqual("passed", Checks->GetPassed(), 1);
	TestEqual("failed", Checks->GetFailed(), 1);
	TestEqual("reported word for word", Reported, TArray<FString>{ TEXT("PASS: yes"), TEXT("FAIL: no") });

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
