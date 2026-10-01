// Copyright IG. All Rights Reserved.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/ClueLogScreen.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"

namespace MvsCaseFileTests
{
	using namespace MvsMenuTest;

	void CaseFile(FMvsScript& Script, const FRig& Rig)
	{
		// The case file's own key (J) closes it.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->ToggleClueLog(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UClueLogScreen>()); }, Open, TEXT("J opens the case file (precondition)"))
			.Do([]() { SendKey(EKeys::J); })
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UClueLogScreen>()); }, Quick, TEXT("J again closes the case file"));

		// Review finding 9: the case file follows a changed list of the same length.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->ToggleClueLog(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UClueLogScreen>()); }, Open, TEXT("the case file opens again (precondition)"))
			.Do([Rig]()
			{
				UClueListViewModel* Clues = Rig.ViewModels() ? Rig.ViewModels()->GetClues() : nullptr;
				if (!Clues || Clues->GetEntries().Num() < 2)
				{
					Rig.Check(false, TEXT("the level has at least two clues (precondition)"));
					return;
				}
				TArray<TObjectPtr<UClueEntryViewModel>> Reversed = Clues->GetEntries();
				Algo::Reverse(Reversed);
				Clues->SetEntries(Reversed);
				const UMvsClueTileView* Tiles = FindIn<UMvsClueTileView>(ActiveScreen<UClueLogScreen>());
				Rig.Check(Tiles && Tiles->GetItemAt(0) == Reversed[0], TEXT("the case file shows a reordered clue list (review 9)"));
				Algo::Reverse(Reversed);
				Clues->SetEntries(Reversed);
			})
			.Do([]() { SendKey(EKeys::Virtual_Gamepad_Back.GetVirtualKey()); })
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UClueLogScreen>()); }, Quick, TEXT("the gamepad's back button closes the case file (review 11)"));
	}
}

// The case file's keys open and close it, and it follows its view model.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalCaseFileTest, "Mvs.Functional.CaseFile", MvsMenuTest::Flags)
bool FMvsFunctionalCaseFileTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("CaseFile"), &MvsCaseFileTests::CaseFile);
	return true;
}

#endif
