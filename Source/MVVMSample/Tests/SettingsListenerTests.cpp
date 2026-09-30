// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "Engine/GameInstance.h"
#include "ViewModels/SettingsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamSettingsListenerTest, "Gotham.Settings.Listener",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamSettingsListenerTest::RunTest(const FString& Parameters)
{
	// An uninitialised subsystem is enough (the listener only touches its delegate); it must live in a game instance.
	UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
	UGothamSettingsSubsystem* Settings = NewObject<UGothamSettingsSubsystem>(GameInstance);
	// Any concrete UObject works as the owner that scopes the callback.
	UObject* Owner = NewObject<USettingsViewModel>();
	const FGothamSettingsData Data;
	int32 Calls = 0;
	auto Count = [&Calls](const FGothamSettingsData&) { ++Calls; };

	{
		FGothamSettingsListener Listener;
		TestFalse("no subsystem: binding fails", Listener.Bind(nullptr, Owner, Count));
		TestTrue("binds", Listener.Bind(Settings, Owner, Count) && Listener.IsBound());
		Settings->OnSettingsChanged.Broadcast(Data);
		TestEqual("called on broadcast", Calls, 1);

		// Binding again replaces the subscription instead of adding a second one.
		Listener.Bind(Settings, Owner, Count);
		Settings->OnSettingsChanged.Broadcast(Data);
		TestEqual("rebinding never doubles the calls", Calls, 2);

		Listener.Reset();
		Settings->OnSettingsChanged.Broadcast(Data);
		TestEqual("no call after Reset", Calls, 2);
		TestFalse("not bound after Reset", Listener.IsBound());
		Listener.Reset(); // repeated reset is harmless

		Listener.Bind(Settings, Owner, Count);
	}
	Settings->OnSettingsChanged.Broadcast(Data);
	TestEqual("no call after the listener is destroyed", Calls, 2);
	TestFalse("destruction removed the delegate binding", Settings->OnSettingsChanged.IsBound());

	// The callback is scoped to its owner: once the owner is gone it is skipped, even if nobody called Reset.
	{
		FGothamSettingsListener Listener;
		UObject* ShortLived = NewObject<USettingsViewModel>();
		Listener.Bind(Settings, ShortLived, Count);
		ShortLived->MarkAsGarbage();
		Settings->OnSettingsChanged.Broadcast(Data);
		TestEqual("a dead owner's callback does not run", Calls, 2);
	}

	// The subsystem going first is also fine: Reset and destruction just do nothing.
	{
		FGothamSettingsListener Listener;
		UGothamSettingsSubsystem* Gone = NewObject<UGothamSettingsSubsystem>(GameInstance);
		Listener.Bind(Gone, Owner, Count);
		Gone->MarkAsGarbage(); // weak pointers to a garbage object are already invalid; no GC pass needed
		TestFalse("a destroyed subsystem leaves the listener unbound", Listener.IsBound());
		Listener.Reset();
	}
	Owner->MarkAsGarbage();
	Settings->MarkAsGarbage();
	GameInstance->MarkAsGarbage();
	return true;
}

#endif
