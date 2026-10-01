// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamInputGlyph.h"

#include "Accessibility/GothamSettingsListener.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/GothamPanel.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Input/GothamActionSource.h"
#include "UI/GothamWidgetTick.h"
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
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer))
	{
		InputMethodHandle = Input->OnInputMethodChangedNative.AddUObject(this, &UGothamInputGlyph::HandleInputMethodChanged);
	}
	// Keys are known only once Enhanced Input has built its mappings, and change when the player rebinds.
	if (auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		Enhanced->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UGothamInputGlyph::HandleMappingsRebuilt);
	}
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { Refresh(); });
	Refresh();
}

void UGothamInputGlyph::NativeDestruct()
{
	SettingsListener.Reset();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer))
	{
		Input->OnInputMethodChangedNative.Remove(InputMethodHandle);
	}
	if (auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		Enhanced->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UGothamInputGlyph::HandleMappingsRebuilt);
	}
	Super::NativeDestruct();
}

void UGothamInputGlyph::SetAction(FName InActionName)
{
	ActionName = InActionName;
	Action = nullptr;
	Refresh();
}

void UGothamInputGlyph::SetInputAction(const UInputAction* InAction)
{
	ActionName = NAME_None;
	Action = InAction;
	Refresh();
}

FText UGothamInputGlyph::GetKeyLabel(const FKey& Key, const ULocalPlayer* Player)
{
	const UCommonInputSubsystem* Input = Player ? UCommonInputSubsystem::Get(Player) : nullptr;
	return GothamBindings::GetKeyLabel(Key, Input ? GothamBindings::GamepadStyleFromName(Input->GetCurrentGamepadName()) : EGothamGamepadStyle::Xbox);
}

FKey UGothamInputGlyph::FindKey(const ULocalPlayer* Player, const UInputAction* InAction)
{
	const UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(Player);
	return Input ? FindKey(Player, InAction, Input->GetCurrentInputType()) : FKey();
}

FKey UGothamInputGlyph::FindKey(const ULocalPlayer* Player, const UInputAction* InAction, ECommonInputType InputType)
{
	// Touch has no keys of its own; Common UI's prompts fall back to the keyboard there as well.
	return CommonUI::GetFirstKeyForInputType(Player, InputType == ECommonInputType::Touch ? ECommonInputType::MouseAndKeyboard : InputType, InAction);
}

FKey UGothamInputGlyph::FindKeyForAction(const APlayerController* Player, FName InActionName)
{
	return Player ? FindKey(Player->GetLocalPlayer(), IGothamActionSource::Find(Player, InActionName)) : FKey();
}

void UGothamInputGlyph::Refresh()
{
	if (!Text)
	{
		return;
	}
	if (!Action && !ActionName.IsNone())
	{
		Action = IGothamActionSource::Find(GetOwningPlayer(), ActionName);
	}

	const FKey Key = FindKey(GetOwningLocalPlayer(), Action);
	Text->SetText(Key.IsValid() ? GetKeyLabel(Key, GetOwningLocalPlayer()) : FText::GetEmpty());
	// Palette tokens, so the key caps follow high contrast like everything else.
	const FLinearColor Ink = GothamStyle::Token(this, EGothamColorToken::TextPrimary);
	Text->SetColorAndOpacity(Ink);
	Frame->SetColors(FLinearColor(Ink.R, Ink.G, Ink.B, 0.12f), FLinearColor(Ink.R, Ink.G, Ink.B, 0.55f));
	// Key caps get one cut corner; gamepad face buttons read as round-ish octagons.
	const bool bFaceButton = Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right
		|| Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_FaceButton_Top;
	Frame->SetShape(bFaceButton ? 8.f : 4.f, bFaceButton ? EGothamChamfer::All : EGothamChamfer::BottomRight);
}
