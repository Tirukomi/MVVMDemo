// Copyright Epic Games, Inc. All Rights Reserved.

#include "Accessibility/GothamSettingsListener.h"

#include "Accessibility/GothamSettingsSubsystem.h"

bool FGothamSettingsListener::Bind(UObject* Owner, FCallback Callback)
{
	return Bind(UGothamSettingsSubsystem::Get(Owner), Owner, MoveTemp(Callback));
}

bool FGothamSettingsListener::Bind(UGothamSettingsSubsystem* InSettings, UObject* Owner, FCallback Callback)
{
	Reset();
	if (!InSettings || !Owner || !Callback)
	{
		return false;
	}
	Settings = InSettings;
	Handle = InSettings->OnSettingsChanged.AddWeakLambda(Owner, [Callback = MoveTemp(Callback)](const FGothamSettingsData& Data)
	{
		Callback(Data);
	});
	return true;
}

void FGothamSettingsListener::Reset()
{
	if (UGothamSettingsSubsystem* Subsystem = Settings.Get())
	{
		Subsystem->OnSettingsChanged.Remove(Handle);
	}
	Settings.Reset();
	Handle.Reset();
}
