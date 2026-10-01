// Copyright IG. All Rights Reserved.

#include "ViewModels/MvsViewModelResolver.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "ViewModels/MvsViewModelSubsystem.h"

UObject* UMvsViewModelResolver::CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const
{
	if (UserWidget && ExpectedType)
	{
		if (ULocalPlayer* LocalPlayer = UserWidget->GetOwningLocalPlayer())
		{
			if (auto* Subsystem = LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>())
			{
				return Subsystem->FindViewModel(ExpectedType);
			}
		}
	}
	return nullptr;
}
