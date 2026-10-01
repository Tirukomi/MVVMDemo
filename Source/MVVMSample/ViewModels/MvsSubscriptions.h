// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "UObject/WeakObjectPtr.h"

/**
 * A set of native multicast-delegate subscriptions that remove themselves. Keep one as a member and add to it:
 *
 *     Subscriptions.Add(Health, &UHealthComponent::OnHealthChanged, this, [this](float Health, float Max) { ... });
 *     ... later:  Subscriptions.Reset();   (or let the destructor do it)
 *
 * Each callback is scoped to Listener (it never runs once Listener is gone), and each source is held weakly, so the
 * order in which sources and listeners go away does not matter. No delegate handles to track by hand.
 */
class FMvsSubscriptions
{
public:
	FMvsSubscriptions() = default;
	~FMvsSubscriptions() { Reset(); }
	FMvsSubscriptions(const FMvsSubscriptions&) = delete;
	FMvsSubscriptions& operator=(const FMvsSubscriptions&) = delete;

	/** Subscribes Functor to Source->*Delegate. A null Source is ignored. */
	template<typename TSource, typename TDelegate, typename TFunctor>
	void Add(TSource* Source, TDelegate TSource::* Delegate, UObject* Listener, TFunctor&& Functor)
	{
		if (!Source)
		{
			return;
		}
		const FDelegateHandle Handle = (Source->*Delegate).AddWeakLambda(Listener, Forward<TFunctor>(Functor));
		Removers.Add([WeakSource = TWeakObjectPtr<TSource>(Source), Delegate, Handle]()
		{
			if (TSource* Alive = WeakSource.Get())
			{
				(Alive->*Delegate).Remove(Handle);
			}
		});
	}

	/** Unsubscribes everything. Safe to call repeatedly, and after the sources are gone. */
	void Reset()
	{
		for (const TFunction<void()>& Remove : Removers)
		{
			Remove();
		}
		Removers.Reset();
	}

	int32 Num() const { return Removers.Num(); }

private:
	TArray<TFunction<void()>> Removers;
};
