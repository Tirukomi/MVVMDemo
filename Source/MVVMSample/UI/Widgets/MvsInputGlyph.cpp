// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsInputGlyph.h"

#include "Accessibility/MvsSettingsListener.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/MvsPanel.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Input/MvsActionSource.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Widgets/MvsText.h"
#include "Engine/LocalPlayer.h"
#include "Input/MvsBindings.h"
#include "EnhancedInputSubsystems.h"

TSharedRef<SWidget> UMvsInputGlyph::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Frame = WidgetTree->ConstructWidget<UMvsPanel>();
		Frame->SetPanelPadding(FMargin(7.f, 1.f));
		WidgetTree->RootWidget = Frame;

		Text = WidgetTree->ConstructWidget<UMvsText>();
		MvsStyle::ApplyText(Text, EMvsTextStyle::Key, MvsStyle::Token(this, EMvsColorToken::TextPrimary));
		Text->SetJustification(ETextJustify::Center);
		Frame->SetContent(Text);
	}
	return Super::RebuildWidget();
}

void UMvsInputGlyph::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer))
	{
		InputMethodHandle = Input->OnInputMethodChangedNative.AddUObject(this, &UMvsInputGlyph::HandleInputMethodChanged);
	}
	// Keys are known only once Enhanced Input has built its mappings, and change when the player rebinds.
	if (auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		Enhanced->ControlMappingsRebuiltDelegate.AddUniqueDynamic(this, &UMvsInputGlyph::HandleMappingsRebuilt);
	}
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { Refresh(); });
	Refresh();
}

void UMvsInputGlyph::NativeDestruct()
{
	SettingsListener.Reset();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer))
	{
		Input->OnInputMethodChangedNative.Remove(InputMethodHandle);
	}
	if (auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr)
	{
		Enhanced->ControlMappingsRebuiltDelegate.RemoveDynamic(this, &UMvsInputGlyph::HandleMappingsRebuilt);
	}
	Super::NativeDestruct();
}

void UMvsInputGlyph::SetAction(FName InActionName)
{
	ActionName = InActionName;
	Action = nullptr;
	Refresh();
}

void UMvsInputGlyph::SetInputAction(const UInputAction* InAction)
{
	ActionName = NAME_None;
	Action = InAction;
	Refresh();
}

FText UMvsInputGlyph::GetKeyLabel(const FKey& Key, const ULocalPlayer* Player)
{
	const UCommonInputSubsystem* Input = Player ? UCommonInputSubsystem::Get(Player) : nullptr;
	return MvsBindings::GetKeyLabel(Key, Input ? MvsBindings::GamepadStyleFromName(Input->GetCurrentGamepadName()) : EMvsGamepadStyle::Xbox);
}

FKey UMvsInputGlyph::FindKey(const ULocalPlayer* Player, const UInputAction* InAction)
{
	const UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(Player);
	return Input ? FindKey(Player, InAction, Input->GetCurrentInputType()) : FKey();
}

FKey UMvsInputGlyph::FindKey(const ULocalPlayer* Player, const UInputAction* InAction, ECommonInputType InputType)
{
	// Touch has no keys of its own; Common UI's prompts fall back to the keyboard there as well.
	return CommonUI::GetFirstKeyForInputType(Player, InputType == ECommonInputType::Touch ? ECommonInputType::MouseAndKeyboard : InputType, InAction);
}

FKey UMvsInputGlyph::FindKeyForAction(const APlayerController* Player, FName InActionName)
{
	return Player ? FindKey(Player->GetLocalPlayer(), IMvsActionSource::Find(Player, InActionName)) : FKey();
}

void UMvsInputGlyph::Refresh()
{
	if (!Text)
	{
		return;
	}
	if (!Action && !ActionName.IsNone())
	{
		Action = IMvsActionSource::Find(GetOwningPlayer(), ActionName);
	}

	const FKey Key = FindKey(GetOwningLocalPlayer(), Action);
	Text->SetText(Key.IsValid() ? GetKeyLabel(Key, GetOwningLocalPlayer()) : FText::GetEmpty());
	// Palette tokens, so the key caps follow high contrast like everything else.
	const FLinearColor Ink = MvsStyle::Token(this, EMvsColorToken::TextPrimary);
	Text->SetColorAndOpacity(Ink);
	Frame->SetColors(FLinearColor(Ink.R, Ink.G, Ink.B, 0.12f), FLinearColor(Ink.R, Ink.G, Ink.B, 0.55f));
	// Key caps get one cut corner; gamepad face buttons read as round-ish octagons.
	const bool bFaceButton = Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right
		|| Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_FaceButton_Top;
	Frame->SetShape(bFaceButton ? 8.f : 4.f, bFaceButton ? EMvsChamfer::All : EMvsChamfer::BottomRight);
}
