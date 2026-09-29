// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Gameplay/ClueDataAsset.h"
#include "Gameplay/DetectiveComponent.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/ObjectivesViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GothamDetectiveTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;

	UClueDataAsset* MakeClue(const TCHAR* Id, const TCHAR* Title)
	{
		UClueDataAsset* Clue = NewObject<UClueDataAsset>(GetTransientPackage());
		Clue->ClueId = Id;
		Clue->Title = FText::FromString(Title);
		Clue->Description = FText::FromString(TEXT("Body"));
		return Clue;
	}
}

// Toggling starts an eased transition; the alpha reaches 1 and back to 0 and the view model tracks visibility.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamDetectiveTransitionTest, "Gotham.Detective.Transition", GothamDetectiveTests::Flags)
bool FGothamDetectiveTransitionTest::RunTest(const FString& Parameters)
{
	UDetectiveComponent* Detective = NewObject<UDetectiveComponent>(GetTransientPackage());
	UDetectiveViewModel* VM = NewObject<UDetectiveViewModel>(GetTransientPackage());
	Detective->OnDetectiveChanged.AddLambda([VM](bool bActive, float Alpha) { VM->SetState(bActive, Alpha); });

	TestFalse("starts off", VM->GetIsVisible());

	Detective->ToggleDetective();
	TestTrue("active immediately", VM->GetIsActive());
	TestTrue("visible as soon as it turns on", VM->GetIsVisible());
	TestEqual("alpha has not moved yet", VM->GetAlpha(), 0.f);

	Detective->Advance(0.1f);
	TestTrue("alpha is rising", VM->GetAlpha() > 0.f && VM->GetAlpha() < 1.f);

	Detective->Advance(5.f);
	TestEqual("alpha reaches 1", VM->GetAlpha(), 1.f);

	Detective->ToggleDetective();
	TestFalse("inactive right away", VM->GetIsActive());
	TestTrue("still visible while fading out", VM->GetIsVisible());

	Detective->Advance(5.f);
	TestEqual("alpha returns to 0", VM->GetAlpha(), 0.f);
	TestFalse("hidden once faded", VM->GetIsVisible());
	return true;
}

// Scanning cannot happen outside detective mode, and a clue is only ever recorded once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamClueScanTest, "Gotham.Detective.Scanning", GothamDetectiveTests::Flags)
bool FGothamClueScanTest::RunTest(const FString& Parameters)
{
	UDetectiveComponent* Detective = NewObject<UDetectiveComponent>(GetTransientPackage());
	const UClueDataAsset* A = GothamDetectiveTests::MakeClue(TEXT("A"), TEXT("Clue A"));

	int32 ScanEvents = 0;
	Detective->OnClueScanned.AddLambda([&ScanEvents](const UClueDataAsset*) { ++ScanEvents; });

	TestFalse("cannot scan while detective mode is off", Detective->TryScan());

	Detective->RegisterScan(A);
	Detective->RegisterScan(A);
	TestEqual("a clue is announced once", ScanEvents, 1);
	TestEqual("scanned count", Detective->GetScannedCount(), 1);
	TestTrue("clue is recorded as scanned", Detective->IsScanned(TEXT("A")));
	TestFalse("other clues are not", Detective->IsScanned(TEXT("B")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamObjectivesTest, "Gotham.ViewModels.Objectives.Progress", GothamDetectiveTests::Flags)
bool FGothamObjectivesTest::RunTest(const FString& Parameters)
{
	UObjectivesViewModel* VM = NewObject<UObjectivesViewModel>(GetTransientPackage());

	VM->SetProgress(0, 0);
	TestEqual("no clues means no progress", VM->GetProgressPercent(), 0.f);
	TestFalse("an empty objective is not complete", VM->GetIsComplete());

	VM->SetProgress(2, 5);
	TestEqual("two of five", VM->GetProgressPercent(), 0.4f);
	TestEqual("progress text is formatted by the view model", VM->GetProgressText().ToString(), FString(TEXT("2 / 5")));
	TestFalse("not complete yet", VM->GetIsComplete());

	VM->SetProgress(9, 5);
	TestEqual("found is clamped to total", VM->GetFoundCount(), 5);
	TestTrue("complete when everything is found", VM->GetIsComplete());
	return true;
}

// Undiscovered clues never leak their real text; discovery flips it and is only counted once.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamClueListTest, "Gotham.ViewModels.Clues.Discovery", GothamDetectiveTests::Flags)
bool FGothamClueListTest::RunTest(const FString& Parameters)
{
	UClueListViewModel* List = NewObject<UClueListViewModel>(GetTransientPackage());
	TArray<TObjectPtr<UClueEntryViewModel>> Entries;
	for (const TCHAR* Id : { TEXT("A"), TEXT("B"), TEXT("C") })
	{
		UClueEntryViewModel* Entry = NewObject<UClueEntryViewModel>(GetTransientPackage());
		Entry->Initialize(Id, FText::FromString(FString::Printf(TEXT("Title %s"), Id)), FText::FromString(TEXT("Body")), TSoftObjectPtr<UTexture2D>());
		Entries.Add(Entry);
	}
	List->SetEntries(MoveTemp(Entries));

	TestEqual("three clues", List->GetTotalCount(), 3);
	TestEqual("none discovered", List->GetDiscoveredCount(), 0);
	TestEqual("hidden title", List->GetEntries()[0]->GetDisplayTitle().ToString(), FString(TEXT("???")));

	TestTrue("first discovery counts", List->MarkDiscovered(TEXT("A")));
	TestFalse("rediscovery does not", List->MarkDiscovered(TEXT("A")));
	TestFalse("unknown clue is rejected", List->MarkDiscovered(TEXT("Z")));
	TestEqual("one discovered", List->GetDiscoveredCount(), 1);
	TestEqual("real title after discovery", List->GetEntries()[0]->GetDisplayTitle().ToString(), FString(TEXT("Title A")));
	TestEqual("others stay hidden", List->GetEntries()[1]->GetDisplayTitle().ToString(), FString(TEXT("???")));

	// Re-supplying the entries (e.g. after rebinding) must keep the count of already-discovered ones.
	TArray<TObjectPtr<UClueEntryViewModel>> Same = List->GetEntries();
	List->SetEntries(Same);
	TestEqual("discovered count survives SetEntries", List->GetDiscoveredCount(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
