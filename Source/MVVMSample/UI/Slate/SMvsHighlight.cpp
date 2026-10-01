// Copyright IG. All Rights Reserved.

#include "UI/Slate/SMvsHighlight.h"

void SMvsHighlight::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	SetVisibility(EVisibility::HitTestInvisible);
}

void SMvsHighlight::SetTarget(const TSharedPtr<SWidget>& InTarget, bool bSnap)
{
	Target = InTarget;
	bSnapNext = bSnapNext || bSnap;
	EnsureTimer();
}

void SMvsHighlight::SetActive(bool bInActive)
{
	if (bActive != bInActive)
	{
		bActive = bInActive;
		EnsureTimer();
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SMvsHighlight::SetLook(const FMvsPanelLook& InLook)
{
	Look = InLook;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SMvsHighlight::EnsureTimer()
{
	if (!Timer.IsValid())
	{
		Timer = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SMvsHighlight::Update));
	}
}

bool SMvsHighlight::ReadTargetRect(FVector2f& OutPosition, FVector2f& OutSize) const
{
	const TSharedPtr<SWidget> Pinned = Target.Pin();
	if (!Pinned.IsValid())
	{
		return false;
	}
	const FGeometry& Mine = GetTickSpaceGeometry();
	const FGeometry& Theirs = Pinned->GetTickSpaceGeometry();
	const FVector2f TheirSize = FVector2f(Theirs.GetLocalSize());
	if (TheirSize.X <= 0.f || TheirSize.Y <= 0.f || Mine.Scale <= 0.f)
	{
		return false;
	}
	OutPosition = FVector2f(Mine.AbsoluteToLocal(Theirs.GetAbsolutePosition()));
	OutSize = FVector2f(Theirs.GetAbsoluteSize()) / Mine.Scale;
	return true;
}

EActiveTimerReturnType SMvsHighlight::Update(double InCurrentTime, float InDeltaTime)
{
	FVector2f Position, Size;
	if (!ReadTargetRect(Position, Size))
	{
		// No layout yet (first frame of a new screen): keep waiting while the list is active.
		if (!bActive)
		{
			Timer.Reset();
			return EActiveTimerReturnType::Stop;
		}
		return EActiveTimerReturnType::Continue;
	}

	const FVector2f OldPosition = Slide.Position;
	const FVector2f OldSize = Slide.Size;
	const bool bHadValue = Slide.HasValue();
	Slide.SetTarget(Position, Size, bSnapNext || bReducedMotion);
	bSnapNext = false;
	Slide.Advance(InDeltaTime);
	if (!bHadValue || !Slide.Position.Equals(OldPosition, 0.01f) || !Slide.Size.Equals(OldSize, 0.01f))
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	// While the list holds focus keep following (reflow, scrolling); otherwise stop once settled.
	if (bActive || Slide.IsMoving())
	{
		return EActiveTimerReturnType::Continue;
	}
	Timer.Reset();
	return EActiveTimerReturnType::Stop;
}

int32 SMvsHighlight::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (!Slide.HasValue())
	{
		return LayerId;
	}
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A * (bActive ? 1.f : 0.35f);
	MvsPaintPanel(OutDrawElements, LayerId, AllottedGeometry, Slide.Position, Slide.Size, Look, Opacity);
	return LayerId + 1;
}
