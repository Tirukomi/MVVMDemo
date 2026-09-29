// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Style/GothamMotion.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Components/Widget.h"
#include "Containers/Ticker.h"

namespace
{
	enum class EChannel : uint8 { Scale, Opacity, Translation };

	/** Active animations per widget and channel, so a new animation replaces the old one instead of stacking. */
	TMap<TPair<TWeakObjectPtr<UWidget>, EChannel>, FTSTicker::FDelegateHandle>& Active()
	{
		static TMap<TPair<TWeakObjectPtr<UWidget>, EChannel>, FTSTicker::FDelegateHandle> Map;
		return Map;
	}

	void Run(UWidget* Widget, EChannel Channel, float Seconds, TFunction<void(UWidget*, float)> Apply)
	{
		const TPair<TWeakObjectPtr<UWidget>, EChannel> Key(Widget, Channel);
		if (FTSTicker::FDelegateHandle* Existing = Active().Find(Key))
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*Existing);
		}
		const TWeakObjectPtr<UWidget> Weak(Widget);
		TSharedRef<float> Elapsed = MakeShared<float>(0.f);
		Apply(Widget, 0.f);
		Active().Add(Key, FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak, Elapsed, Seconds, Apply, Key](float Dt)
		{
			UWidget* W = Weak.Get();
			if (!W)
			{
				Active().Remove(Key);
				return false;
			}
			*Elapsed += Dt;
			const float T = FMath::Clamp(*Elapsed / FMath::Max(Seconds, KINDA_SMALL_NUMBER), 0.f, 1.f);
			Apply(W, T);
			if (T >= 1.f)
			{
				Active().Remove(Key);
				return false;
			}
			return true;
		})));
	}
}

namespace GothamMotion
{
	float PunchCurve(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		// Fast rise to 1 at T = 0.25, then ease back out to 0.
		return T < 0.25f ? FMath::InterpEaseOut(0.f, 1.f, T / 0.25f, 2.f) : FMath::InterpEaseInOut(1.f, 0.f, (T - 0.25f) / 0.75f, 2.f);
	}

	bool IsReduced(const UObject* Context)
	{
		const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(Context);
		return Settings && Settings->GetSettings().bReducedMotion;
	}

	void Pop(UWidget* Widget, float Peak, float Seconds)
	{
		if (!Widget || IsReduced(Widget))
		{
			return;
		}
		Widget->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
		Run(Widget, EChannel::Scale, Seconds, [Peak](UWidget* W, float T)
		{
			const float S = 1.f + (Peak - 1.f) * PunchCurve(T);
			W->SetRenderScale(FVector2D(S, S));
		});
	}

	void Fade(UWidget* Widget, float From, float To, float Seconds)
	{
		if (!Widget)
		{
			return;
		}
		if (IsReduced(Widget))
		{
			Widget->SetRenderOpacity(To);
			return;
		}
		Run(Widget, EChannel::Opacity, Seconds, [From, To](UWidget* W, float T)
		{
			W->SetRenderOpacity(FMath::Lerp(From, To, FMath::InterpEaseOut(0.f, 1.f, T, 2.f)));
		});
	}

	void SlideIn(UWidget* Widget, const FVector2D& From, float Seconds)
	{
		if (!Widget)
		{
			return;
		}
		if (IsReduced(Widget))
		{
			Widget->SetRenderTranslation(FVector2D::ZeroVector);
			return;
		}
		Run(Widget, EChannel::Translation, Seconds, [From](UWidget* W, float T)
		{
			W->SetRenderTranslation(From * (1.f - FMath::InterpEaseOut(0.f, 1.f, T, 3.f)));
		});
	}
}

void FGothamSlideRect::SetTarget(const FVector2f& InPosition, const FVector2f& InSize, bool bSnap)
{
	TargetPosition = InPosition;
	TargetSize = InSize;
	if (bSnap || !bHasValue)
	{
		Position = TargetPosition;
		Size = TargetSize;
		bHasValue = true;
		bMoving = false;
		return;
	}
	bMoving = !Position.Equals(TargetPosition, 0.5f) || !Size.Equals(TargetSize, 0.5f);
}

bool FGothamSlideRect::Advance(float DeltaSeconds)
{
	if (!bMoving)
	{
		return false;
	}
	// Frame-rate independent: the remaining distance shrinks by exp(-Rate * dt) every step.
	const float Keep = FMath::Exp(-Rate * FMath::Max(DeltaSeconds, 0.f));
	Position = TargetPosition + (Position - TargetPosition) * Keep;
	Size = TargetSize + (Size - TargetSize) * Keep;
	if (Position.Equals(TargetPosition, 0.5f) && Size.Equals(TargetSize, 0.5f))
	{
		Position = TargetPosition;
		Size = TargetSize;
		bMoving = false;
	}
	return bMoving;
}
