// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GothamViewModelSubsystem.h"

#include "Core/GothamCharacter.h"
#include "Engine/LocalPlayer.h"
#include "MVVMViewModelBase.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamViewModelBinders.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/ThreatViewModel.h"

template<typename TBinder>
TBinder* UGothamViewModelSubsystem::AddBinder()
{
	TBinder* Binder = NewObject<TBinder>(this);
	Binder->Initialize(*GetLocalPlayer());
	Binders.Add(Binder);
	return Binder;
}

void UGothamViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AddBinder<UGothamVitalsBinder>();
	AddBinder<UGothamGadgetBinder>();
	AddBinder<UGothamComboBinder>();
	AddBinder<UGothamDetectiveBinder>();
	AddBinder<UGothamThreatBinder>();
	ClueBinder = AddBinder<UGothamClueBinder>();
}

void UGothamViewModelSubsystem::Deinitialize()
{
	for (UGothamViewModelBinder* Binder : Binders)
	{
		Binder->Deinitialize();
	}
	Binders.Reset();
	ClueBinder = nullptr;
	Super::Deinitialize();
}

void UGothamViewModelSubsystem::BindToCharacter(AGothamCharacter* Character)
{
	for (UGothamViewModelBinder* Binder : Binders)
	{
		Binder->Unbind();
		if (Character)
		{
			Binder->Bind(*Character);
		}
	}
}

UObject* UGothamViewModelSubsystem::FindViewModel(const UClass* ViewModelClass) const
{
	for (const UGothamViewModelBinder* Binder : Binders)
	{
		for (UMVVMViewModelBase* ViewModel : Binder->GetViewModels())
		{
			if (ViewModel && ViewModelClass && ViewModel->GetClass()->IsChildOf(ViewModelClass))
			{
				return ViewModel;
			}
		}
	}
	return nullptr;
}

UPlayerVitalsViewModel* UGothamViewModelSubsystem::GetVitals() const { return Get<UPlayerVitalsViewModel>(); }
UGadgetBarViewModel* UGothamViewModelSubsystem::GetGadgetBar() const { return Get<UGadgetBarViewModel>(); }
UComboViewModel* UGothamViewModelSubsystem::GetCombo() const { return Get<UComboViewModel>(); }
UDetectiveViewModel* UGothamViewModelSubsystem::GetDetective() const { return Get<UDetectiveViewModel>(); }
UObjectivesViewModel* UGothamViewModelSubsystem::GetObjectives() const { return Get<UObjectivesViewModel>(); }
UClueListViewModel* UGothamViewModelSubsystem::GetClues() const { return Get<UClueListViewModel>(); }
USubtitleViewModel* UGothamViewModelSubsystem::GetSubtitles() const { return Get<USubtitleViewModel>(); }
UThreatViewModel* UGothamViewModelSubsystem::GetThreats() const { return Get<UThreatViewModel>(); }

void UGothamViewModelSubsystem::AddDebugClues(int32 Count)
{
	if (ClueBinder)
	{
		ClueBinder->AddDebugClues(Count);
	}
}
