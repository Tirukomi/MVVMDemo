// Copyright IG. All Rights Reserved.

#include "ViewModels/MvsViewModelSubsystem.h"

#include "Core/MvsCharacter.h"
#include "Engine/LocalPlayer.h"
#include "MVVMViewModelBase.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/ForensicViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsViewModelBinders.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/ThreatViewModel.h"

template<typename TBinder>
TBinder* UMvsViewModelSubsystem::AddBinder()
{
	TBinder* Binder = NewObject<TBinder>(this);
	Binder->Initialize(*GetLocalPlayer());
	Binders.Add(Binder);
	return Binder;
}

void UMvsViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AddBinder<UMvsVitalsBinder>();
	AddBinder<UMvsGadgetBinder>();
	AddBinder<UMvsComboBinder>();
	AddBinder<UMvsForensicBinder>();
	AddBinder<UMvsThreatBinder>();
	ClueBinder = AddBinder<UMvsClueBinder>();
}

void UMvsViewModelSubsystem::Deinitialize()
{
	for (UMvsViewModelBinder* Binder : Binders)
	{
		Binder->Deinitialize();
	}
	Binders.Reset();
	ClueBinder = nullptr;
	Super::Deinitialize();
}

void UMvsViewModelSubsystem::BindToCharacter(AMvsCharacter* Character)
{
	for (UMvsViewModelBinder* Binder : Binders)
	{
		Binder->Unbind();
		if (Character)
		{
			Binder->Bind(*Character);
		}
	}
}

UObject* UMvsViewModelSubsystem::FindViewModel(const UClass* ViewModelClass) const
{
	for (const UMvsViewModelBinder* Binder : Binders)
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

UPlayerVitalsViewModel* UMvsViewModelSubsystem::GetVitals() const { return Get<UPlayerVitalsViewModel>(); }
UGadgetBarViewModel* UMvsViewModelSubsystem::GetGadgetBar() const { return Get<UGadgetBarViewModel>(); }
UComboViewModel* UMvsViewModelSubsystem::GetCombo() const { return Get<UComboViewModel>(); }
UForensicViewModel* UMvsViewModelSubsystem::GetForensic() const { return Get<UForensicViewModel>(); }
UObjectivesViewModel* UMvsViewModelSubsystem::GetObjectives() const { return Get<UObjectivesViewModel>(); }
UClueListViewModel* UMvsViewModelSubsystem::GetClues() const { return Get<UClueListViewModel>(); }
USubtitleViewModel* UMvsViewModelSubsystem::GetSubtitles() const { return Get<USubtitleViewModel>(); }
UThreatViewModel* UMvsViewModelSubsystem::GetThreats() const { return Get<UThreatViewModel>(); }

#if !UE_BUILD_SHIPPING
void UMvsViewModelSubsystem::AddDebugClues(int32 Count)
{
	if (ClueBinder)
	{
		ClueBinder->AddDebugClues(Count);
	}
}
#endif
