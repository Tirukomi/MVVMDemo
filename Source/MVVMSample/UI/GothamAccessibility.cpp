// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamAccessibility.h"

#include "Components/Widget.h"
#include "Layout/Children.h"
#include "Widgets/SWidget.h"

namespace GothamAccessibility
{
	void SetText(const TSharedPtr<SWidget>& Widget, TAttribute<FText> Text)
	{
#if WITH_ACCESSIBILITY
		if (Widget.IsValid())
		{
			Widget->SetAccessibleBehavior(EAccessibleBehavior::Custom, MoveTemp(Text));
			Widget->SetCanChildrenBeAccessible(false);
		}
#endif
	}

	TSharedPtr<SWidget> FindButton(const UWidget& Widget)
	{
		// Breadth-first: the Common UI button sits a few levels under the user widget's root.
		TArray<TSharedPtr<SWidget>> Queue = { Widget.GetCachedWidget() };
		for (int32 i = 0; i < Queue.Num(); ++i)
		{
			const TSharedPtr<SWidget> Current = Queue[i];
			if (!Current.IsValid())
			{
				continue;
			}
			if (Current->GetType() == TEXT("SCommonButton"))
			{
				return Current;
			}
			FChildren* Children = Current->GetChildren();
			for (int32 c = 0; Children && c < Children->Num(); ++c)
			{
				Queue.Add(Children->GetChildAt(c));
			}
		}
		return nullptr;
	}

	FText GetText(const TSharedPtr<SWidget>& Widget)
	{
#if WITH_ACCESSIBILITY
		return Widget.IsValid() ? Widget->GetAccessibleText() : FText::GetEmpty();
#else
		return FText::GetEmpty();
#endif
	}
}
