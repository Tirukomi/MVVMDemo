// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/MvsSubscriptions.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// MvsMVVM::Bind subscribes exactly the listed fields, and Unbind removes everything the owner subscribed.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsMVVMBindTest, "Mvs.ViewModels.BindHelper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsMVVMBindTest::RunTest(const FString& Parameters)
{
	using FVM = UPlayerVitalsViewModel::FFieldNotificationClassDescriptor;
	UPlayerVitalsViewModel* VM = NewObject<UPlayerVitalsViewModel>(GetTransientPackage());
	// Any concrete UObject works as the owner that scopes the handler.
	UObject* Owner = NewObject<USubtitleViewModel>(GetTransientPackage());
	TArray<FName> Seen;
	const auto Record = MvsMVVM::FDelegate::CreateWeakLambda(Owner, [&Seen](UObject*, MvsMVVM::FFieldId Field)
	{
		Seen.Add(Field.GetName());
	});

	MvsMVVM::Bind(nullptr, Record, { FVM::HealthPercent });
	MvsMVVM::Unbind(nullptr, Owner);
	TestTrue("null view model is a no-op", Seen.IsEmpty());

	MvsMVVM::Bind(VM, Record, { FVM::HealthPercent, FVM::bIsLowHealth });
	VM->SetVitals(50.f, 100.f);
	TestTrue("bound field notifies", Seen.Contains(FVM::HealthPercent.GetName()));
	TestFalse("unlisted field does not notify", Seen.Contains(FVM::Health.GetName()));

	Seen.Reset();
	VM->SetVitals(10.f, 100.f);
	TestTrue("second bound field notifies", Seen.Contains(FVM::bIsLowHealth.GetName()));

	// A second handler on the same owner: Unbind still removes both.
	int32 Extra = 0;
	MvsMVVM::Bind(VM, MvsMVVM::FDelegate::CreateWeakLambda(Owner, [&Extra](UObject*, MvsMVVM::FFieldId) { ++Extra; }),
		{ FVM::Health });
	VM->SetVitals(20.f, 100.f);
	TestEqual("second handler notifies", Extra, 1);

	MvsMVVM::Unbind(VM, Owner);
	Seen.Reset();
	VM->SetVitals(90.f, 100.f);
	TestTrue("no calls after Unbind", Seen.IsEmpty() && Extra == 1);
	return true;
}

// The gadget bar's "use" command, and FMvsSubscriptions on it: listener-scoped, removed by Reset, safe after the
// source is gone. The binders rely on all three.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsSubscriptionsTest, "Mvs.ViewModels.Subscriptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FMvsSubscriptionsTest::RunTest(const FString& Parameters)
{
	UGadgetBarViewModel* Bar = NewObject<UGadgetBarViewModel>(GetTransientPackage());
	Bar->SetSlotCount(3);
	UObject* Listener = NewObject<USubtitleViewModel>(GetTransientPackage());
	TArray<int32> Used;
	{
		FMvsSubscriptions Subscriptions;
		Subscriptions.Add(Bar, &UGadgetBarViewModel::OnUseRequested, Listener, [&Used](int32 Slot) { Used.Add(Slot); });
		Subscriptions.Add(static_cast<UGadgetBarViewModel*>(nullptr), &UGadgetBarViewModel::OnUseRequested, Listener, [](int32) {});
		TestEqual("a null source is ignored", Subscriptions.Num(), 1);

		Bar->RequestUse(1);
		Bar->RequestUse(7);
		TestTrue("the command reaches the subscriber, for valid slots only", Used.Num() == 1 && Used[0] == 1);

		Subscriptions.Reset();
		Bar->RequestUse(2);
		TestEqual("Reset unsubscribes", Used.Num(), 1);
		TestFalse("and leaves nothing on the delegate", Bar->OnUseRequested.IsBound());

		Subscriptions.Add(Bar, &UGadgetBarViewModel::OnUseRequested, Listener, [&Used](int32 Slot) { Used.Add(Slot); });
	}
	TestFalse("going out of scope unsubscribes", Bar->OnUseRequested.IsBound());

	FMvsSubscriptions Late;
	Late.Add(Bar, &UGadgetBarViewModel::OnUseRequested, Listener, [](int32) {});
	Bar->MarkAsGarbage();
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	Late.Reset();
	TestEqual("Reset after the source is gone is safe", Late.Num(), 0);
	return true;
}

#endif
