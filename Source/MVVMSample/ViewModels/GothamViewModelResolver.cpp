// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GothamViewModelResolver.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "ViewModels/GothamViewModelSubsystem.h"

UObject* UGothamViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (UserWidget && ExpectedType)
	{
		if (ULocalPlayer* LocalPlayer = UserWidget->GetOwningLocalPlayer())
		{
			if (auto* Subsystem = LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>())
			{
				return Subsystem->FindViewModel(ExpectedType);
			}
		}
	}
	return nullptr;
}
