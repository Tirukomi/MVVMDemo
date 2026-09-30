// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "INotifyFieldValueChanged.h"
#include <initializer_list>

/**
 * One-line field-notify subscriptions for code-built widgets:
 *
 *     GothamMVVM::Unbind(ViewModel, this);
 *     ViewModel = InViewModel;
 *     GothamMVVM::Bind(ViewModel, this, &UMyWidget::OnFieldChanged, { FVM::Health, FVM::bIsLowHealth });
 *
 * Every listed field goes to the same handler. A widget that routes some fields elsewhere (a per-frame fast path,
 * say) calls Bind once per handler. Unbind removes everything Owner subscribed on that view model, for any handler.
 * Both are no-ops on a null view model.
 */
namespace GothamMVVM
{
	using FFieldId = UE::FieldNotification::FFieldId;
	using FDelegate = INotifyFieldValueChanged::FFieldValueChangedDelegate;

	MVVMSAMPLE_API void Bind(INotifyFieldValueChanged* ViewModel, const FDelegate& Delegate, std::initializer_list<FFieldId> Fields);
	MVVMSAMPLE_API void Unbind(INotifyFieldValueChanged* ViewModel, const UObject* Owner);

	template <typename OwnerClass, typename HandlerClass>
	void Bind(INotifyFieldValueChanged* ViewModel, OwnerClass* Owner, void (HandlerClass::*Handler)(UObject*, FFieldId),
		std::initializer_list<FFieldId> Fields)
	{
		if (ViewModel && Owner)
		{
			Bind(ViewModel, FDelegate::CreateUObject(Owner, Handler), Fields);
		}
	}
}
