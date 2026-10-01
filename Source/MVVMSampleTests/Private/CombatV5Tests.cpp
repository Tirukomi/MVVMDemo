// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Gameplay/ThreatTypes.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/ThreatViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamV5Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamThugBrainTest, "Gotham.Combat.ThugBrain", GothamV5Tests::Flags)
bool FGothamThugBrainTest::RunTest(const FString& Parameters)
{
	FGothamThugBrain Brain;
	TestFalse("cannot counter an idle thug", Brain.Counter());
	TestTrue("idle thugs can start a warning", Brain.BeginWarning());
	TestFalse("but not twice", Brain.BeginWarning());

	TestEqual("no event mid-warning", Brain.Advance(FGothamThugBrain::WarningSeconds * 0.5f), EGothamThugEvent::None);
	TestNearlyEqual("progress runs 0..1 across the warning", Brain.GetWarningProgress(), 0.5f, 0.01f);
	TestEqual("an uncountered warning strikes", Brain.Advance(FGothamThugBrain::WarningSeconds), EGothamThugEvent::Strike);
	TestEqual("then recovers", Brain.State, EGothamThugState::Recover);
	TestFalse("no counter after the strike", Brain.Counter());
	TestEqual("and is ready again", Brain.Advance(FGothamThugBrain::RecoverSeconds + 0.01f), EGothamThugEvent::Ready);

	Brain.BeginWarning();
	Brain.Advance(0.3f);
	TestTrue("a counter lands during the warning", Brain.Counter());
	TestEqual("and stuns", Brain.State, EGothamThugState::Stunned);
	TestEqual("a stunned thug never strikes", Brain.Advance(FGothamThugBrain::WarningSeconds + 0.1f), EGothamThugEvent::None);
	TestEqual("stun wears off", Brain.Advance(FGothamThugBrain::StunSeconds), EGothamThugEvent::Ready);
	TestEqual("no progress outside a warning", Brain.GetWarningProgress(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamAttackDirectorTest, "Gotham.Combat.AttackDirector", GothamV5Tests::Flags)
bool FGothamAttackDirectorTest::RunTest(const FString& Parameters)
{
	FGothamAttackDirector Director(1234);
	const TArray<float> Near = { 400.f, 600.f, 5000.f };
	TArray<EGothamThugState> States = { EGothamThugState::Idle, EGothamThugState::Idle, EGothamThugState::Idle };

	TestEqual("waits out the first breather", Director.Advance(Director.MinGapSeconds * 0.5f, States, Near), INDEX_NONE);
	const int32 First = Director.Advance(Director.MinGapSeconds, States, Near);
	TestTrue("then picks a thug in range", First == 0 || First == 1);
	TestTrue("and starts a new breather within the configured gap",
		Director.GetCooldown() >= Director.MinGapSeconds && Director.GetCooldown() <= Director.MaxGapSeconds);

	// While someone warns, nobody else is picked, however long it takes.
	States[First] = EGothamThugState::Warning;
	for (int32 i = 0; i < 20; ++i)
	{
		TestEqual("one telegraph at a time", Director.Advance(1.f, States, Near), INDEX_NONE);
	}
	TestTrue("the breather restarts after a warning", Director.GetCooldown() >= Director.MinGapSeconds);

	// Out of range thugs never attack.
	FGothamAttackDirector Far(99);
	const TArray<EGothamThugState> OneIdle = { EGothamThugState::Idle };
	for (int32 i = 0; i < 20; ++i)
	{
		TestEqual("thugs out of reach stay put", Far.Advance(1.f, OneIdle, TArray<float>{ Far.EngageRange + 1.f }), INDEX_NONE);
	}
	// Stunned or recovering thugs are not candidates.
	const TArray<EGothamThugState> Busy = { EGothamThugState::Stunned, EGothamThugState::Recover };
	TestEqual("busy thugs are skipped", Far.Advance(10.f, Busy, TArray<float>{ 100.f, 100.f }), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamEdgeArrowTest, "Gotham.Combat.EdgeArrow", GothamV5Tests::Flags)
bool FGothamEdgeArrowTest::RunTest(const FString& Parameters)
{
	const FVector2D Viewport(1920.f, 1080.f);
	constexpr float Inset = 40.f;
	FVector2D Pos;
	float Angle = 0.f;

	GothamThreat::EdgeArrow(FVector(1.f, 1.f, 0.f), Viewport, Inset, Pos, Angle);
	TestTrue("to the right: middle of the right edge", Pos.Equals(FVector2D(1920.f - Inset, 540.f), 0.5f));
	TestNearlyEqual("pointing right", Angle, 0.f, 0.001f);

	GothamThreat::EdgeArrow(FVector(1.f, 0.f, 1.f), Viewport, Inset, Pos, Angle);
	TestTrue("above: middle of the top edge", Pos.Equals(FVector2D(960.f, Inset), 0.5f));
	TestNearlyEqual("pointing up", Angle, -HALF_PI, 0.001f);

	GothamThreat::EdgeArrow(FVector(-1.f, 0.f, 0.f), Viewport, Inset, Pos, Angle);
	TestTrue("straight behind: bottom edge ('behind you')", Pos.Equals(FVector2D(960.f, 1080.f - Inset), 0.5f));

	GothamThreat::EdgeArrow(FVector(-1.f, -3.f, -3.f), Viewport, Inset, Pos, Angle);
	const bool bOnEdge = FMath::IsNearlyEqual(Pos.X, Inset, 0.5f) || FMath::IsNearlyEqual(Pos.Y, 1080.f - Inset, 0.5f);
	TestTrue("diagonal: on the inset rectangle, never past it", bOnEdge && Pos.X >= Inset - 0.5f && Pos.Y <= 1080.f - Inset + 0.5f);
	TestTrue("down-left points down-left", Angle > HALF_PI && Angle < PI);

	TestTrue("centre is on screen", GothamThreat::IsOnScreen(FVector2D(960.f, 540.f), Viewport, 24.f));
	TestFalse("inside the margin is not", GothamThreat::IsOnScreen(FVector2D(10.f, 540.f), Viewport, 24.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamComboMilestoneTest, "Gotham.Combat.ComboMilestone", GothamV5Tests::Flags)
bool FGothamComboMilestoneTest::RunTest(const FString& Parameters)
{
	TestEqual("9 -> 10 reaches 10", GothamCombo::MilestoneReached(9, 10), 10);
	TestEqual("10 -> 11 reaches nothing new", GothamCombo::MilestoneReached(10, 11), 0);
	TestEqual("a jump 8 -> 21 reports the latest", GothamCombo::MilestoneReached(8, 21), 20);
	TestEqual("dropping never counts", GothamCombo::MilestoneReached(15, 0), 0);
	TestEqual("0 is not a milestone", GothamCombo::MilestoneReached(0, 3), 0);

	UComboViewModel* VM = NewObject<UComboViewModel>();
	for (int32 Hits = 1; Hits <= 25; ++Hits)
	{
		VM->SetCombo(Hits, 1.f, 1.f);
	}
	TestEqual("the view model bumps once per milestone", VM->GetMilestoneCount(), 2);
	TestFalse("with its callout text", VM->GetMilestoneText().IsEmpty());
	VM->SetCombo(0, 1.f, 0.f);
	VM->SetCombo(10, 1.f, 1.f);
	TestEqual("a new streak can reach it again", VM->GetMilestoneCount(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamTraumaTest, "Gotham.Combat.CameraTrauma", GothamV5Tests::Flags)
bool FGothamTraumaTest::RunTest(const FString& Parameters)
{
	FGothamTrauma Trauma;
	Trauma.Add(0.5f);
	TestNearlyEqual("shake is trauma squared (small hits barely move the camera)", Trauma.GetShake(), 0.25f, 0.001f);
	Trauma.Add(5.f);
	TestEqual("trauma caps at 1", Trauma.Trauma, 1.f);
	TestTrue("drains while positive", Trauma.Advance(0.1f) && Trauma.Trauma < 1.f);
	TestFalse("and stops at zero", Trauma.Advance(10.f));
	TestEqual("with nothing left", Trauma.GetShake(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamThreatViewModelTest, "Gotham.Combat.ThreatViewModel", GothamV5Tests::Flags)
bool FGothamThreatViewModelTest::RunTest(const FString& Parameters)
{
	UThreatViewModel* VM = NewObject<UThreatViewModel>();
	int32 Notifications = 0;
	VM->AddFieldValueChangedDelegate(UThreatViewModel::FFieldNotificationClassDescriptor::WarningCount,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&Notifications](UObject*, UE::FieldNotification::FFieldId) { ++Notifications; }));

	TArray<FGothamThreatSnapshot> Snaps;
	Snaps.AddDefaulted(3);
	Snaps[1].State = EGothamThugState::Warning;
	VM->SetThreats(Snaps);
	TestEqual("counts threats", VM->GetThreatCount(), 3);
	TestEqual("counts warnings", VM->GetWarningCount(), 1);
	VM->SetThreats(Snaps);
	TestEqual("per-frame updates with no change do not re-notify", Notifications, 1);
	return true;
}

#endif
