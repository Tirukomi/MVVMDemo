// Copyright IG. All Rights Reserved.

#include "ViewModels/MvsMVVM.h"

namespace MvsMVVM
{
	void Bind(INotifyFieldValueChanged* ViewModel, const FDelegate& Delegate, std::initializer_list<FFieldId> Fields)
	{
		if (!ViewModel || !Delegate.IsBound())
		{
			return;
		}
		for (const FFieldId& Field : Fields)
		{
			ViewModel->AddFieldValueChangedDelegate(Field, Delegate);
		}
	}

	void Unbind(INotifyFieldValueChanged* ViewModel, const UObject* Owner)
	{
		if (ViewModel && Owner)
		{
			ViewModel->RemoveAllFieldValueChangedDelegates(Owner);
		}
	}
}
