// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamHintButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "UI/Widgets/GothamText.h"

UGothamHintButton::UGothamHintButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UGothamButtonStyle::StaticClass();
	SetIsFocusable(false);
}

bool UGothamHintButton::Initialize()
{
	// As UGothamButton: Common UI wires its internal button only if the tree already has a root.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		WidgetTree->RootWidget = Row;

		Glyph = WidgetTree->ConstructWidget<UGothamInputGlyph>();
		Row->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);

		Label = WidgetTree->ConstructWidget<UGothamText>();
		GothamStyle::ApplyText(Label, EGothamTextStyle::Label, GothamStyle::Token(this, EGothamColorToken::TextMuted));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		if (KeyboardKey.IsValid() || GamepadKey.IsValid())
		{
			SetHint(KeyboardKey, GamepadKey, PendingLabel);
		}
	}
	return Super::Initialize();
}

void UGothamHintButton::SetHint(const FKey& InKeyboardKey, const FKey& InGamepadKey, const FText& InLabel)
{
	KeyboardKey = InKeyboardKey;
	GamepadKey = InGamepadKey;
	PendingLabel = InLabel;
	if (Glyph)
	{
		Glyph->SetFixedKeys(KeyboardKey, GamepadKey);
		Label->SetText(PendingLabel);
		Label->SetVisibility(PendingLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UGothamHintButton::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyState(); });
	ApplyState();
}

void UGothamHintButton::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamHintButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bHoveredNow = GothamUI::HoverEnabled();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	FocusBeforePointer = FSlateApplication::Get().GetUserFocusedWidget(LocalPlayer ? LocalPlayer->GetControllerId() : 0);
	ApplyState();
}

void UGothamHintButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bHoveredNow = false;
	bPressedNow = false;
	ApplyState();
}

void UGothamHintButton::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPressedNow = true;
	ApplyState();
}

void UGothamHintButton::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPressedNow = false;
	ApplyState();
}

void UGothamHintButton::ApplyState()
{
	if (!Label)
	{
		return;
	}
	// Hover brightens the label to the accent so it reads as clickable; press dims the whole prompt a touch.
	Label->SetColorAndOpacity(GothamStyle::Token(this, bHoveredNow ? EGothamColorToken::Accent : EGothamColorToken::TextMuted));
	SetRenderOpacity(bPressedNow ? 0.7f : 1.f);
}
