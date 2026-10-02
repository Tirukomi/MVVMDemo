// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Slate/MvsWheelTypes.h"
#include "UI/Slate/SComboMeter.h"
#include "UI/Slate/SThreatIndicatorLayer.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsSlateTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

// Segment 0 is "up", the rest go clockwise. Screen Y grows downward, so "up" is negative Y.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsWheelHitTest, "Mvs.Slate.Wheel.HitTesting", MvsSlateTests::Flags)
bool FMvsWheelHitTest::RunTest(const FString& Parameters)
{
	constexpr float DeadZone = 50.f;
	TestEqual("4 segments: up is 0", MvsWheel::IndexFromOffset(FVector2D(0, -100), 4, DeadZone), 0);
	TestEqual("4 segments: right is 1", MvsWheel::IndexFromOffset(FVector2D(100, 0), 4, DeadZone), 1);
	TestEqual("4 segments: down is 2", MvsWheel::IndexFromOffset(FVector2D(0, 100), 4, DeadZone), 2);
	TestEqual("4 segments: left is 3", MvsWheel::IndexFromOffset(FVector2D(-100, 0), 4, DeadZone), 3);

	// 3 gadgets: centres at 0, 120, 240 degrees; boundaries at 60, 180, 300.
	TestEqual("3 segments: just clockwise of up stays 0", MvsWheel::IndexFromOffset(FVector2D(60, -100), 3, DeadZone), 0);
	TestEqual("3 segments: lower right is 1", MvsWheel::IndexFromOffset(FVector2D(100, 60), 3, DeadZone), 1);
	TestEqual("3 segments: lower left is 2", MvsWheel::IndexFromOffset(FVector2D(-100, 60), 3, DeadZone), 2);
	TestEqual("3 segments: just anticlockwise of up wraps to 0", MvsWheel::IndexFromOffset(FVector2D(-60, -100), 3, DeadZone), 0);

	TestEqual("inside the dead zone selects nothing", MvsWheel::IndexFromOffset(FVector2D(10, 10), 4, DeadZone), static_cast<int32>(INDEX_NONE));
	TestEqual("no items selects nothing", MvsWheel::IndexFromOffset(FVector2D(0, -100), 0, DeadZone), static_cast<int32>(INDEX_NONE));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsWheelAngleTest, "Mvs.Slate.Wheel.Angles", MvsSlateTests::Flags)
bool FMvsWheelAngleTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual("up is 0 degrees", MvsWheel::AngleFromTopDeg(FVector2D(0, -1)), 0.f, 0.01f);
	TestNearlyEqual("right is 90 degrees", MvsWheel::AngleFromTopDeg(FVector2D(1, 0)), 90.f, 0.01f);
	TestNearlyEqual("down is 180 degrees", MvsWheel::AngleFromTopDeg(FVector2D(0, 1)), 180.f, 0.01f);
	TestNearlyEqual("left is 270 degrees", MvsWheel::AngleFromTopDeg(FVector2D(-1, 0)), 270.f, 0.01f);
	TestNearlyEqual("segment centres are evenly spaced", MvsWheel::SegmentCenterDeg(2, 6), 120.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsComboMeterTest, "Mvs.Slate.ComboMeter.LitSegments", MvsSlateTests::Flags)
bool FMvsComboMeterTest::RunTest(const FString& Parameters)
{
	TestEqual("empty lights nothing", SComboMeter::GetLitSegments(0.f, 10), 0);
	TestEqual("full lights everything", SComboMeter::GetLitSegments(1.f, 10), 10);
	TestEqual("half lights five", SComboMeter::GetLitSegments(0.5f, 10), 5);
	TestEqual("a sliver still lights one segment", SComboMeter::GetLitSegments(0.01f, 10), 1);
	TestEqual("over-range is clamped", SComboMeter::GetLitSegments(3.f, 10), 10);
	return true;
}

// Second review 29: a world overlay repaints only when its items changed or it animates, not every frame while active.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsWorldOverlayRepaintTest, "Mvs.Slate.WorldOverlay.Repaint", MvsSlateTests::Flags)
bool FMvsWorldOverlayRepaintTest::RunTest(const FString& Parameters)
{
	TSharedRef<SThreatIndicatorLayer> Layer = SNew(SThreatIndicatorLayer);
	FMvsThreatIndicator Thug;
	Thug.bProjected = true;
	Thug.Screen = FVector2D(400.0, 300.0);
	Layer->SetProvider([&Thug](TArray<FMvsThreatIndicator>& Out) { Out.Add(Thug); });

	TestTrue("a new item repaints", Layer->Refresh(1.0));
	TestFalse("the same item next frame does not", Layer->Refresh(1.1));
	Thug.Screen.X += 3.0;
	TestTrue("a moved item repaints", Layer->Refresh(1.2));
	TestFalse("and is still next frame", Layer->Refresh(1.3));

	Thug.bWarning = true;
	TestTrue("a warning repaints", Layer->Refresh(1.4));
	TestTrue("and keeps repainting while it pulses", Layer->Refresh(1.5));
	Layer->SetReducedMotion(true);
	TestFalse("under reduced motion a warning does not pulse, so it does not repaint unchanged", Layer->Refresh(1.6));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
