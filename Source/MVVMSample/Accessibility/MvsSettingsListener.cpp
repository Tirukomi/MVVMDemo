// Copyright IG. All Rights Reserved.

#include "Accessibility/MvsSettingsListener.h"

#include "Accessibility/MvsSettingsSubsystem.h"

bool FMvsSettingsListener::Bind(UObject* Owner, FCallback Callback)
{
	return Bind(UMvsSettingsSubsystem::Get(Owner), Owner, MoveTemp(Callback));
}

bool FMvsSettingsListener::Bind(UMvsSettingsSubsystem* InSettings, UObject* Owner, FCallback Callback)
{
	Reset();
	if (!InSettings || !Owner || !Callback)
	{
		return false;
	}
	Settings = InSettings;
	Handle = InSettings->OnSettingsChanged.AddWeakLambda(Owner, [Callback = MoveTemp(Callback)](const FMvsSettingsData& Data)
	{
		Callback(Data);
	});
	return true;
}

void FMvsSettingsListener::Reset()
{
	if (UMvsSettingsSubsystem* Subsystem = Settings.Get())
	{
		Subsystem->OnSettingsChanged.Remove(Handle);
	}
	Settings.Reset();
	Handle.Reset();
}
