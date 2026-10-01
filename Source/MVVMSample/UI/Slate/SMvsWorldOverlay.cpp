// Copyright IG. All Rights Reserved.

#include "UI/Slate/SMvsWorldOverlay.h"

void SMvsWorldOverlayBase::ConstructOverlay()
{
	SetCanTick(false);
	SetVisibility(EVisibility::HitTestInvisible);
}

void SMvsWorldOverlayBase::SetActive(bool bInActive)
{
	if (bInActive && !Timer.IsValid())
	{
		Timer = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SMvsWorldOverlayBase::OnRefreshTimer));
	}
	else if (!bInActive && Timer.IsValid())
	{
		UnRegisterActiveTimer(Timer.ToSharedRef());
		Timer.Reset();
		ClearItems();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

EActiveTimerReturnType SMvsWorldOverlayBase::OnRefreshTimer(double InCurrentTime, float InDeltaTime)
{
	RefreshTime = InCurrentTime;
	RefreshItems();
	Invalidate(EInvalidateWidgetReason::Paint);
	return EActiveTimerReturnType::Continue;
}
