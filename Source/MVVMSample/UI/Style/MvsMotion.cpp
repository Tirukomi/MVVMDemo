// Copyright IG. All Rights Reserved.

#include "UI/Style/MvsMotion.h"

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Components/Widget.h"
#include "Types/ISlateMetaData.h"
#include "Widgets/SWidget.h"

namespace
{
	enum class EChannel : uint8 { Scale, Opacity, Translation, Count };

	/**
	 * The animations running on one Slate widget, one per channel, kept on the widget itself so a new animation can
	 * replace the old one instead of stacking. Gone with the widget: nothing global outlives it.
	 */
	class FMvsMotionMetaData : public ISlateMetaData
	{
	public:
		SLATE_METADATA_TYPE(FMvsMotionMetaData, ISlateMetaData)

		TWeakPtr<FActiveTimerHandle> Channels[static_cast<int32>(EChannel::Count)];
	};

	/**
	 * Runs Apply(Widget, T) with T from 0 to 1 over Seconds, as an active timer on the widget's Slate widget: it ticks
	 * with Slate in real time (so the wheel's slow motion does not slow it) and only while the animation runs.
	 */
	void Run(UWidget* Widget, EChannel Channel, float Seconds, TFunction<void(UWidget*, float)> Apply)
	{
		const TSharedPtr<SWidget> Slate = Widget->GetCachedWidget();
		if (!Slate.IsValid())
		{
			Apply(Widget, 1.f); // not on screen yet: nothing to animate, so land on the end state
			return;
		}
		TSharedPtr<FMvsMotionMetaData> Motion = Slate->GetMetaData<FMvsMotionMetaData>();
		if (!Motion.IsValid())
		{
			Motion = MakeShared<FMvsMotionMetaData>();
			Slate->AddMetadata(Motion.ToSharedRef());
		}
		TWeakPtr<FActiveTimerHandle>& Running = Motion->Channels[static_cast<int32>(Channel)];
		if (const TSharedPtr<FActiveTimerHandle> Previous = Running.Pin())
		{
			Slate->UnRegisterActiveTimer(Previous.ToSharedRef());
		}

		Apply(Widget, 0.f);
		const TWeakObjectPtr<UWidget> Weak(Widget);
		const TSharedRef<float> Elapsed = MakeShared<float>(0.f);
		Running = Slate->RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateLambda(
			[Weak, Elapsed, Seconds, Apply = MoveTemp(Apply)](double, float DeltaTime)
			{
				UWidget* W = Weak.Get();
				if (!W)
				{
					return EActiveTimerReturnType::Stop;
				}
				*Elapsed += DeltaTime;
				const float T = FMath::Clamp(*Elapsed / FMath::Max(Seconds, KINDA_SMALL_NUMBER), 0.f, 1.f);
				Apply(W, T);
				return T >= 1.f ? EActiveTimerReturnType::Stop : EActiveTimerReturnType::Continue;
			}));
	}
}

namespace MvsMotion
{
	float PunchCurve(float T)
	{
		T = FMath::Clamp(T, 0.f, 1.f);
		// Fast rise to 1 at T = 0.25, then ease back out to 0.
		return T < 0.25f ? FMath::InterpEaseOut(0.f, 1.f, T / 0.25f, 2.f) : FMath::InterpEaseInOut(1.f, 0.f, (T - 0.25f) / 0.75f, 2.f);
	}

	bool IsReduced(const UObject* Context)
	{
		const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(Context);
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

void FMvsSlideRect::SetTarget(const FVector2f& InPosition, const FVector2f& InSize, bool bSnap)
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

bool FMvsSlideRect::Advance(float DeltaSeconds)
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
