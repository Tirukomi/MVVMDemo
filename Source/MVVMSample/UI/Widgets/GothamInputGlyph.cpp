// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamInputGlyph.h"

#include "Accessibility/GothamSettingsListener.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/GothamPanel.h"
#include "Components/TextBlock.h"
#include "Core/GothamPlayerController.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

TSharedRef<SWidget> UGothamInputGlyph::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Frame = WidgetTree->ConstructWidget<UGothamPanel>();
		Frame->SetPanelPadding(FMargin(7.f, 1.f));
		WidgetTree->RootWidget = Frame;

		Text = WidgetTree->ConstructWidget<UTextBlock>();
		GothamStyle::ApplyText(Text, EGothamTextStyle::Key, GothamStyle::Token(this, EGothamColorToken::TextPrimary));
		Text->SetJustification(ETextJustify::Center);
		Frame->SetContent(Text);
	}
	return Super::RebuildWidget();
}

void UGothamInputGlyph::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputMethodHandle = Input->OnInputMethodChangedNative.AddUObject(this, &UGothamInputGlyph::HandleInputMethodChanged);
	}
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (auto* UI = LocalPlayer->GetSubsystem<UGothamUISubsystem>())
		{
			BindingsHandle = UI->OnBindingsChanged.AddUObject(this, &UGothamInputGlyph::Refresh);
		}
	}
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { Refresh(); });
	Refresh();
}

void UGothamInputGlyph::NativeDestruct()
{
	SettingsListener.Reset();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		Input->OnInputMethodChangedNative.Remove(InputMethodHandle);
	}
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (auto* UI = LocalPlayer->GetSubsystem<UGothamUISubsystem>())
		{
			UI->OnBindingsChanged.Remove(BindingsHandle);
		}
	}
	FTSTicker::GetCoreTicker().RemoveTicker(RetryHandle);
	Super::NativeDestruct();
}

void UGothamInputGlyph::SetAction(FName InActionName)
{
	ActionName = InActionName;
	RetriesLeft = 10;
	Refresh();
}

void UGothamInputGlyph::SetFixedKeys(FKey InKeyboardMouseKey, FKey InGamepadKey)
{
	FixedKeyboardMouse = InKeyboardMouseKey;
	FixedGamepad = InGamepadKey;
	Refresh();
}

FText UGothamInputGlyph::GetKeyLabel(const FKey& Key)
{
	static const TMap<FName, const TCHAR*> ShortNames = {
		{ EKeys::Gamepad_FaceButton_Bottom.GetFName(), TEXT("A") },
		{ EKeys::Gamepad_FaceButton_Right.GetFName(), TEXT("B") },
		{ EKeys::Gamepad_FaceButton_Left.GetFName(), TEXT("X") },
		{ EKeys::Gamepad_FaceButton_Top.GetFName(), TEXT("Y") },
		{ EKeys::Gamepad_Special_Right.GetFName(), TEXT("Menu") },
		{ EKeys::Gamepad_RightTrigger.GetFName(), TEXT("RT") },
		{ EKeys::Gamepad_LeftTrigger.GetFName(), TEXT("LT") },
		{ EKeys::Gamepad_RightShoulder.GetFName(), TEXT("RB") },
		{ EKeys::Gamepad_LeftShoulder.GetFName(), TEXT("LB") },
		{ EKeys::Gamepad_DPad_Up.GetFName(), TEXT("D-pad Up") },
		{ EKeys::Gamepad_DPad_Down.GetFName(), TEXT("D-pad Down") },
		{ EKeys::Gamepad_DPad_Left.GetFName(), TEXT("D-pad Left") },
		{ EKeys::Gamepad_DPad_Right.GetFName(), TEXT("D-pad Right") },
		{ EKeys::Gamepad_Special_Left.GetFName(), TEXT("View") },
		{ EKeys::Escape.GetFName(), TEXT("Esc") },
		{ EKeys::Enter.GetFName(), TEXT("Enter") },
		{ EKeys::LeftMouseButton.GetFName(), TEXT("LMB") },
		{ EKeys::RightMouseButton.GetFName(), TEXT("RMB") },
		{ EKeys::MiddleMouseButton.GetFName(), TEXT("MMB") },
	};
	if (const TCHAR* const* Short = ShortNames.Find(Key.GetFName()))
	{
		return FText::FromString(*Short);
	}
	return Key.GetDisplayName();
}

FKey UGothamInputGlyph::FindKeyForAction(const APlayerController* Player, FName InActionName)
{
	const AGothamPlayerController* PC = Cast<AGothamPlayerController>(Player);
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer);
	const bool bGamepad = Input && Input->GetCurrentInputType() == ECommonInputType::Gamepad;
	const UInputAction* Action = PC ? PC->FindAction(InActionName) : nullptr;
	auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (Action && Enhanced)
	{
		for (const FKey& Candidate : Enhanced->QueryKeysMappedToAction(Action))
		{
			if (Candidate.IsGamepadKey() == bGamepad)
			{
				return Candidate;
			}
		}
	}
	return EKeys::Invalid;
}

void UGothamInputGlyph::Refresh()
{
	if (!Text)
	{
		return;
	}

	UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
	const bool bGamepad = Input && Input->GetCurrentInputType() == ECommonInputType::Gamepad;
	const FKey Key = ActionName.IsNone() ? (bGamepad ? FixedGamepad : FixedKeyboardMouse) : FindKeyForAction(GetOwningPlayer(), ActionName);
	Text->SetText(Key.IsValid() ? GetKeyLabel(Key) : FText::GetEmpty());
	// Palette tokens, so the key caps follow high contrast like everything else.
	const FLinearColor Ink = GothamStyle::Token(this, EGothamColorToken::TextPrimary);
	Text->SetColorAndOpacity(Ink);
	Frame->SetColors(FLinearColor(Ink.R, Ink.G, Ink.B, 0.12f), FLinearColor(Ink.R, Ink.G, Ink.B, 0.55f));
	// Key caps get one cut corner; gamepad face buttons read as round-ish octagons.
	const bool bFaceButton = Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right
		|| Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_FaceButton_Top;
	Frame->SetShape(bFaceButton ? 8.f : 4.f, bFaceButton ? EGothamChamfer::All : EGothamChamfer::BottomRight);

	if (!Key.IsValid() && !ActionName.IsNone())
	{
		ScheduleRetry();
	}
}

void UGothamInputGlyph::ScheduleRetry()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RetryHandle);
	if (RetriesLeft <= 0)
	{
		return;
	}
	RetryHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		--RetriesLeft;
		Refresh();
		return false;
	}), 0.1f);
}
