// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SGothamWorldOverlay.h"

void SGothamWorldOverlayBase::ConstructOverlay()
{
	SetCanTick(false);
	SetVisibility(EVisibility::HitTestInvisible);
}

void SGothamWorldOverlayBase::SetActive(bool bInActive)
{
	if (bInActive && !Timer.IsValid())
	{
		Timer = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGothamWorldOverlayBase::OnRefreshTimer));
	}
	else if (!bInActive && Timer.IsValid())
	{
		UnRegisterActiveTimer(Timer.ToSharedRef());
		Timer.Reset();
		ClearItems();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

EActiveTimerReturnType SGothamWorldOverlayBase::OnRefreshTimer(double InCurrentTime, float InDeltaTime)
{
	RefreshTime = InCurrentTime;
	RefreshItems();
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}
