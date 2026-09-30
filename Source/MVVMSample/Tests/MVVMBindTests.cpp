// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "ViewModels/GothamMVVM.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

// GothamMVVM::Bind subscribes exactly the listed fields, and Unbind removes everything the owner subscribed.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamMVVMBindTest, "Gotham.ViewModels.BindHelper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamMVVMBindTest::RunTest(const FString& Parameters)
{
	using FVM = UPlayerVitalsViewModel::FFieldNotificationClassDescriptor;
	UPlayerVitalsViewModel* VM = NewObject<UPlayerVitalsViewModel>(GetTransientPackage());
	// Any concrete UObject works as the owner that scopes the handler.
	UObject* Owner = NewObject<USubtitleViewModel>(GetTransientPackage());
	TArray<FName> Seen;
	const auto Record = GothamMVVM::FDelegate::CreateWeakLambda(Owner, [&Seen](UObject*, GothamMVVM::FFieldId Field)
	{
		Seen.Add(Field.GetName());
	});

	GothamMVVM::Bind(nullptr, Record, { FVM::HealthPercent });
	GothamMVVM::Unbind(nullptr, Owner);
	TestTrue("null view model is a no-op", Seen.IsEmpty());

	GothamMVVM::Bind(VM, Record, { FVM::HealthPercent, FVM::bIsLowHealth });
	VM->SetVitals(50.f, 100.f);
	TestTrue("bound field notifies", Seen.Contains(FVM::HealthPercent.GetName()));
	TestFalse("unlisted field does not notify", Seen.Contains(FVM::Health.GetName()));

	Seen.Reset();
	VM->SetVitals(10.f, 100.f);
	TestTrue("second bound field notifies", Seen.Contains(FVM::bIsLowHealth.GetName()));

	// A second handler on the same owner: Unbind still removes both.
	int32 Extra = 0;
	GothamMVVM::Bind(VM, GothamMVVM::FDelegate::CreateWeakLambda(Owner, [&Extra](UObject*, GothamMVVM::FFieldId) { ++Extra; }),
		{ FVM::Health });
	VM->SetVitals(20.f, 100.f);
	TestEqual("second handler notifies", Extra, 1);

	GothamMVVM::Unbind(VM, Owner);
	Seen.Reset();
	VM->SetVitals(90.f, 100.f);
	TestTrue("no calls after Unbind", Seen.IsEmpty() && Extra == 1);
	return true;
}

#endif
