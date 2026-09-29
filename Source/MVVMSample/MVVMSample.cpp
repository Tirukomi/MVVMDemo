// Copyright Epic Games, Inc. All Rights Reserved.

#include "MVVMSample.h"

#include "Gameplay/ClueStrings.h"
#include "Modules/ModuleManager.h"

class FMVVMSampleModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// String tables must exist before any asset that references them loads.
		GothamClueStrings::Register();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FMVVMSampleModule, MVVMSample, "MVVMSample" );
