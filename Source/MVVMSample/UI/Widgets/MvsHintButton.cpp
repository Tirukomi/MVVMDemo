// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsHintButton.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputTypeEnum.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/UIActionBinding.h"
#include "UI/MvsAccessibility.h"
#include "InputAction.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "UI/Widgets/MvsText.h"

UMvsHintButton::UMvsHintButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UMvsButtonStyle::StaticClass();
	SetIsFocusable(false);
}

bool UMvsHintButton::Initialize()
{
	// As UMvsButton: Common UI wires its internal button only if the tree already has a root.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		WidgetTree->RootWidget = Row;

		Glyph = WidgetTree->ConstructWidget<UMvsInputGlyph>();
		Row->AddChildToHorizontalBox(Glyph)->SetVerticalAlignment(VAlign_Center);

		Label = WidgetTree->ConstructWidget<UMvsText>();
		MvsStyle::ApplyText(Label, EMvsTextStyle::Label, MvsStyle::Token(this, EMvsColorToken::TextMuted));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		if (Action)
		{
			SetInputAction(Action, PendingLabel);
		}
	}
	return Super::Initialize();
}

void UMvsHintButton::SetInputAction(const UInputAction* InAction, const FText& InLabel)
{
	Action = InAction;
	PendingLabel = InLabel;
	if (Glyph)
	{
		Glyph->SetInputAction(Action);
		Label->SetText(PendingLabel);
		Label->SetVisibility(PendingLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UMvsHintButton::SetRepresentedAction(FUIActionBindingHandle InBindingHandle)
{
	BindingHandle = InBindingHandle;
	const TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(BindingHandle);
	SetInputAction(Binding ? Binding->InputAction.Get() : nullptr, BindingHandle.GetDisplayName());
}

FKey UMvsHintButton::GetKeyboardKey() const
{
	return UMvsInputGlyph::FindKey(GetOwningLocalPlayer(), Action, ECommonInputType::MouseAndKeyboard);
}

void UMvsHintButton::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyState(); });
	ApplyState();
	// "Back (Esc)": the action and the key that does it, in the naming of the device in use.
	MvsAccessibility::SetText(MvsAccessibility::FindButton(*this), TAttribute<FText>::CreateWeakLambda(this, [this]()
	{
		const ULocalPlayer* Player = GetOwningLocalPlayer();
		const FKey Key = UMvsInputGlyph::FindKey(Player, Action);
		const FText KeyName = Key.IsValid() ? UMvsInputGlyph::GetKeyLabel(Key, Player) : FText::GetEmpty();
		return PendingLabel.IsEmpty() ? KeyName : FText::Format(NSLOCTEXT("Mvs.Accessibility", "Prompt", "{0} ({1})"), PendingLabel, KeyName);
	}));
}

void UMvsHintButton::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UMvsHintButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bHoveredNow = MvsUI::HoverEnabled();
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	FocusBeforePointer = FSlateApplication::Get().GetUserFocusedWidget(LocalPlayer ? LocalPlayer->GetControllerId() : 0);
	ApplyState();
}

void UMvsHintButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bHoveredNow = false;
	bPressedNow = false;
	ApplyState();
}

void UMvsHintButton::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPressedNow = true;
	ApplyState();
}

void UMvsHintButton::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPressedNow = false;
	ApplyState();
}

void UMvsHintButton::NativeOnClicked()
{
	if (const TSharedPtr<SWidget> Restore = FocusBeforePointer.Pin())
	{
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		FSlateApplication::Get().SetUserFocus(LocalPlayer ? LocalPlayer->GetControllerId() : 0, Restore, EFocusCause::SetDirectly);
	}
	Super::NativeOnClicked();
	if (const TSharedPtr<FUIActionBinding> Binding = FUIActionBinding::FindBinding(BindingHandle))
	{
		Binding->OnExecuteAction.ExecuteIfBound();
	}
}

void UMvsHintButton::ApplyState()
{
	if (!Label)
	{
		return;
	}
	// Hover brightens the label to the accent so it reads as clickable; press dims the whole prompt a touch.
	Label->SetColorAndOpacity(MvsStyle::Token(this, bHoveredNow ? EMvsColorToken::Accent : EMvsColorToken::TextMuted));
	SetRenderOpacity(bPressedNow ? 0.7f : 1.f);
}
