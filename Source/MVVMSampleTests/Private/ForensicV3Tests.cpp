// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/MvsSettingsTypes.h"
#include "Gameplay/ForensicComponent.h"
#include "Gameplay/ForensicTypes.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "ViewModels/ForensicViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsV3Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsScanPulseTest, "Mvs.Forensic.ScanPulse", MvsV3Tests::Flags)
bool FMvsScanPulseTest::RunTest(const FString& Parameters)
{
	FMvsScanPulse Pulse;
	TestFalse("an unfired pulse does nothing", Pulse.Advance(0.1f));

	Pulse.Fire(FVector(100.f, 0.f, 0.f), 3200.f);
	TestTrue("fires active at full strength", Pulse.bActive && Pulse.Strength == 1.f && Pulse.Radius == 0.f);

	Pulse.Advance(0.25f);
	TestNearlyEqual("expands at its speed", Pulse.Radius, FMvsScanPulse::Speed * 0.25f, 0.01f);
	TestEqual("full strength for the first half", Pulse.Strength, 1.f);

	Pulse.Advance(0.5f);
	TestTrue("fades over the second half", Pulse.Strength > 0.f && Pulse.Strength < 1.f);

	TestFalse("stops at its maximum radius", Pulse.Advance(1.f));
	TestFalse("and is inactive", Pulse.bActive);
	TestEqual("with no strength left", Pulse.Strength, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsAnalysisTest, "Mvs.Forensic.HoldToAnalyse", MvsV3Tests::Flags)
bool FMvsAnalysisTest::RunTest(const FString& Parameters)
{
	FMvsAnalysis Analysis;
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
	UForensicComponent* Forensic = NewObject<UForensicComponent>(GetTransientPackage());
	TestFalse("no analysis outside forensic mode", Forensic->BeginAnalyse());
	Forensic->ToggleForensic();
	TestFalse("no analysis without a clue in range", Forensic->BeginAnalyse());
	TestFalse("not analysing", Forensic->IsAnalysing());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsMarkerRulesTest, "Mvs.Forensic.MarkerRules", MvsV3Tests::Flags)
bool FMvsMarkerRulesTest::RunTest(const FString& Parameters)
{
	TestTrue("closer markers are larger", MvsMarkers::ScaleForDistance(300.f) > MvsMarkers::ScaleForDistance(2000.f));
	TestTrue("scale is capped up close", MvsMarkers::ScaleForDistance(1.f) <= 2.f);
	TestTrue("scale is floored far away", MvsMarkers::ScaleForDistance(1.0e6f) >= 0.6f);

	TestEqual("fully opaque well inside range", MvsMarkers::OpacityForDistance(1000.f, 3500.f), 1.f);
	const float Mid = MvsMarkers::OpacityForDistance(3000.f, 3500.f);
	TestTrue("fading near the edge", Mid > 0.f && Mid < 1.f);
	TestEqual("gone at the edge", MvsMarkers::OpacityForDistance(3500.f, 3500.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsAnalysisViewModelTest, "Mvs.ViewModels.Forensic.Analysis", MvsV3Tests::Flags)
bool FMvsAnalysisViewModelTest::RunTest(const FString& Parameters)
{
	UForensicViewModel* VM = NewObject<UForensicViewModel>(GetTransientPackage());
	VM->SetAnalysis(TEXT("Ledger"), 0.4f);
	TestEqual("target is exposed", VM->GetAnalysisTargetId(), FName(TEXT("Ledger")));
	TestNearlyEqual("progress is exposed", VM->GetAnalysisProgress(), 0.4f, 0.001f);
	VM->SetAnalysis(NAME_None, 0.7f);
	TestEqual("no target means no progress", VM->GetAnalysisProgress(), 0.f);

	FMvsSettingsData Data;
	TestTrue("hold is the default", Data.ScanMode == EMvsScanMode::Hold);
	Data.Cycle(EMvsSetting::ScanMode, 1);
	TestTrue("tap is the accessible alternative", Data.ScanMode == EMvsScanMode::Tap);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
