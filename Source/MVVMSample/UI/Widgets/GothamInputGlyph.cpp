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
#include "UI/Widgets/GothamText.h"
#include "Engine/LocalPlayer.h"
#include "Input/GothamBindings.h"
#include "EnhancedInputSubsystems.h"

TSharedRef<SWidget> UGothamInputGlyph::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Frame = WidgetTree->ConstructWidget<UGothamPanel>();
		Frame->SetPanelPadding(FMargin(7.f, 1.f));
		WidgetTree->RootWidget = Frame;

		Text = WidgetTree->ConstructWidget<UGothamText>();
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

FText UGothamInputGlyph::GetKeyLabel(const FKey& Key, const ULocalPlayer* Player)
{
	const UCommonInputSubsystem* Input = Player ? UCommonInputSubsystem::Get(Player) : nullptr;
	return GothamBindings::GetKeyLabel(Key, Input ? GothamBindings::GamepadStyleFromName(Input->GetCurrentGamepadName()) : EGothamGamepadStyle::Xbox);
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
	Text->SetText(Key.IsValid() ? GetKeyLabel(Key, GetOwningLocalPlayer()) : FText::GetEmpty());
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
