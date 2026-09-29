// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "GothamViewModelResolver.generated.h"

/**
 * Lets a UMG widget's MVVM "Viewmodel Context" resolve to the owning local player's view model,
 * so designers pick this resolver in the editor instead of wiring anything by hand.
 */
UCLASS()
class MVVMSAMPLE_API UGothamViewModelResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
	virtual void DestroyInstance(UObject* ViewModel, const UMVVMView* View) const override {}
};
