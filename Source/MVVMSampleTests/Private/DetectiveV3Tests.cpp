// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsTypes.h"
#include "Gameplay/DetectiveComponent.h"
#include "Gameplay/DetectiveTypes.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "ViewModels/DetectiveViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamV3Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamScanPulseTest, "Gotham.Detective.ScanPulse", GothamV3Tests::Flags)
bool FGothamScanPulseTest::RunTest(const FString& Parameters)
{
	FGothamScanPulse Pulse;
	TestFalse("an unfired pulse does nothing", Pulse.Advance(0.1f));

	Pulse.Fire(FVector(100.f, 0.f, 0.f), 3200.f);
	TestTrue("fires active at full strength", Pulse.bActive && Pulse.Strength == 1.f && Pulse.Radius == 0.f);

	Pulse.Advance(0.25f);
	TestNearlyEqual("expands at its speed", Pulse.Radius, FGothamScanPulse::Speed * 0.25f, 0.01f);
	TestEqual("full strength for the first half", Pulse.Strength, 1.f);

	Pulse.Advance(0.5f);
	TestTrue("fades over the second half", Pulse.Strength > 0.f && Pulse.Strength < 1.f);

	TestFalse("stops at its maximum radius", Pulse.Advance(1.f));
	TestFalse("and is inactive", Pulse.bActive);
	TestEqual("with no strength left", Pulse.Strength, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamAnalysisTest, "Gotham.Detective.HoldToAnalyse", GothamV3Tests::Flags)
bool FGothamAnalysisTest::RunTest(const FString& Parameters)
{
	FGothamAnalysis Analysis;
	TestFalse("not running until begun", Analysis.Advance(1.f, 1.f));

	Analysis.Begin();
	TestFalse("half way is not complete", Analysis.Advance(0.5f, 1.f));
	TestNearlyEqual("progress tracks held time", Analysis.Progress, 0.5f, 0.001f);

	Analysis.Cancel();
	TestEqual("releasing early resets progress", Analysis.Progress, 0.f);
	TestFalse("and stops", Analysis.bRunning);

	Analysis.Begin();
	TestFalse("not yet", Analysis.Advance(0.9f, 1.f));
	TestTrue("completes once the duration is held", Analysis.Advance(0.2f, 1.f));
	TestFalse("reports completion exactly once", Analysis.Advance(0.2f, 1.f));

	// Without a world there are no clues, so the component refuses to start an analysis or scan.
	UDetectiveComponent* Detective = NewObject<UDetectiveComponent>(GetTransientPackage());
	TestFalse("no analysis outside detective mode", Detective->BeginAnalyse());
	Detective->ToggleDetective();
	TestFalse("no analysis without a clue in range", Detective->BeginAnalyse());
	TestFalse("not analysing", Detective->IsAnalysing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamMarkerRulesTest, "Gotham.Detective.MarkerRules", GothamV3Tests::Flags)
bool FGothamMarkerRulesTest::RunTest(const FString& Parameters)
{
	TestTrue("closer markers are larger", GothamMarkers::ScaleForDistance(300.f) > GothamMarkers::ScaleForDistance(2000.f));
	TestTrue("scale is capped up close", GothamMarkers::ScaleForDistance(1.f) <= 2.f);
	TestTrue("scale is floored far away", GothamMarkers::ScaleForDistance(1.0e6f) >= 0.6f);

	TestEqual("fully opaque well inside range", GothamMarkers::OpacityForDistance(1000.f, 3500.f), 1.f);
	const float Mid = GothamMarkers::OpacityForDistance(3000.f, 3500.f);
	TestTrue("fading near the edge", Mid > 0.f && Mid < 1.f);
	TestEqual("gone at the edge", GothamMarkers::OpacityForDistance(3500.f, 3500.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamAnalysisViewModelTest, "Gotham.ViewModels.Detective.Analysis", GothamV3Tests::Flags)
bool FGothamAnalysisViewModelTest::RunTest(const FString& Parameters)
{
	UDetectiveViewModel* VM = NewObject<UDetectiveViewModel>(GetTransientPackage());
	VM->SetAnalysis(TEXT("Ledger"), 0.4f);
	TestEqual("target is exposed", VM->GetAnalysisTargetId(), FName(TEXT("Ledger")));
	TestNearlyEqual("progress is exposed", VM->GetAnalysisProgress(), 0.4f, 0.001f);
	VM->SetAnalysis(NAME_None, 0.7f);
	TestEqual("no target means no progress", VM->GetAnalysisProgress(), 0.f);

	FGothamSettingsData Data;
	TestTrue("hold is the default", Data.ScanMode == EGothamScanMode::Hold);
	Data.Cycle(EGothamSetting::ScanMode, 1);
	TestTrue("tap is the accessible alternative", Data.ScanMode == EGothamScanMode::Tap);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
