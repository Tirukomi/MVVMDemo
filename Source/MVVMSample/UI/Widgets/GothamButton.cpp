// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamPanel.h"
#include "UI/Widgets/GothamText.h"

UGothamButtonStyle::UGothamButtonStyle()
{
	FSlateBrush None;
	None.DrawAs = ESlateBrushDrawType::NoDrawType;

	NormalBase = None;
	NormalHovered = None;
	NormalPressed = None;
	SelectedBase = None;
	SelectedHovered = None;
	SelectedPressed = None;
	Disabled = None;
	ButtonPadding = FMargin(0.f);
}

UGothamButton::UGothamButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UGothamButtonStyle::StaticClass();
	SetIsFocusable(true);
}

bool UGothamButton::Initialize()
{
	// UCommonButtonBase only wires its internal button (click, focus, style) if the widget tree already has a
	// root when it initializes. There is no designer asset here, so build the content first.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);

		Frame = WidgetTree->ConstructWidget<UGothamPanel>();
		WidgetTree->RootWidget = Frame;

		Label = WidgetTree->ConstructWidget<UGothamText>();
		Label->SetText(PendingLabel);
		Frame->SetContent(Label);
		ApplyKind();
	}
	return Super::Initialize();
}

void UGothamButton::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyState(); });
	ApplyState();
}

void UGothamButton::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamButton::SetLabel(const FText& InLabel)
{
	PendingLabel = InLabel;
	if (Label)
	{
		Label->SetText(InLabel);
	}
}

void UGothamButton::SetKind(EGothamButtonKind InKind)
{
	Kind = InKind;
	if (Kind == EGothamButtonKind::Tab)
	{
		SetIsFocusable(false);
	}
	ApplyKind();
	ApplyState();
}

void UGothamButton::ApplyKind()
{
	if (!Frame || !Label)
	{
		return;
	}
	switch (Kind)
	{
	case EGothamButtonKind::MenuItem:
		Label->SetFont(GothamStyle::Font(EGothamTextStyle::Header));
		GothamText::SetUpperCase(Label, true);
		Label->SetJustification(ETextJustify::Left);
		Frame->SetPanelPadding(FMargin(22.f, 9.f, 40.f, 9.f));
		break;
	case EGothamButtonKind::Tab:
		Label->SetFont(GothamStyle::Font(EGothamTextStyle::Label));
		GothamText::SetUpperCase(Label, true);
		Label->SetJustification(ETextJustify::Center);
		Frame->SetPanelPadding(FMargin(20.f, 8.f));
		Frame->SetShape(6.f, EGothamChamfer::Opposite);
		break;
	default:
		Label->SetFont(GothamStyle::Font(EGothamTextStyle::BodyStrong));
		GothamText::SetUpperCase(Label, false);
		Label->SetJustification(ETextJustify::Center);
		Frame->SetPanelPadding(FMargin(22.f, 9.f));
		Frame->SetShape(8.f, EGothamChamfer::Opposite);
		break;
	}
}

void UGothamButton::ApplyState()
{
	if (!Frame || !Label)
	{
		return;
	}
	using namespace GothamStyle;
	const bool bHot = bFocused || bHoveredNow;
	const bool bSelectedNow = GetSelected();
	const FLinearColor Accent = Token(this, Kind == EGothamButtonKind::Danger ? EGothamColorToken::Danger : EGothamColorToken::Accent);
	const FLinearColor Clear = FLinearColor::Transparent;

	switch (Kind)
	{
	case EGothamButtonKind::MenuItem:
		// Focus is drawn by the list's sliding highlight; the item only brightens its label.
		Frame->SetColors(Clear, Clear, 0.f);
		Frame->SetAccent(Clear, 0.f);
		Frame->SetGlow(Clear, 0.f);
		Label->SetColorAndOpacity(Token(this, bHot ? EGothamColorToken::TextPrimary : EGothamColorToken::TextMuted));
		break;

	case EGothamButtonKind::Tab:
		Frame->SetColors(bSelectedNow ? FLinearColor(Accent.R, Accent.G, Accent.B, 0.92f) : Clear,
			bHot && !bSelectedNow ? Token(this, EGothamColorToken::PanelEdge, 0.8f) : Clear, 1.f);
		Frame->SetAccent(Clear, 0.f);
		Frame->SetGlow(Clear, 0.f);
		// Dark text on the selected (accent) tab, so selection never relies on colour alone: it is also a filled block.
		Label->SetColorAndOpacity(bSelectedNow ? Token(this, EGothamColorToken::Panel)
			: Token(this, bHot ? EGothamColorToken::TextPrimary : EGothamColorToken::TextMuted));
		break;

	default:
	{
		const float PanelA = PanelAlpha(this);
		FLinearColor Fill = Token(this, EGothamColorToken::Panel, PanelA);
		if (bPressedNow)
		{
			Fill = FLinearColor::LerpUsingHSV(Fill, Accent, 0.35f);
			Fill.A = PanelA;
		}
		else if (bHot)
		{
			Fill = FMath::Lerp(Fill, FLinearColor(Accent.R, Accent.G, Accent.B, PanelA), 0.1f);
		}
		const FLinearColor Edge = bHot ? Accent
			: Kind == EGothamButtonKind::Danger ? FLinearColor(Accent.R, Accent.G, Accent.B, 0.55f)
			: Token(this, EGothamColorToken::PanelEdge, 0.6f);
		Frame->SetColors(Fill, Edge, bHot ? 1.5f : 1.f);
		Frame->SetAccent(Accent, bHot ? 3.f : 0.f);
		Frame->SetGlow(FLinearColor(Accent.R, Accent.G, Accent.B, 0.3f), bHot ? 6.f : 0.f);
		Label->SetColorAndOpacity(Token(this, bHot ? EGothamColorToken::TextPrimary : EGothamColorToken::TextMuted));
		break;
	}
	}
	SetRenderOpacity(GetIsEnabled() ? 1.f : 0.4f);
}

void UGothamButton::HandleFocusReceived()
{
	Super::HandleFocusReceived();
	bFocused = true;
	ApplyState();
}

void UGothamButton::HandleFocusLost()
{
	Super::HandleFocusLost();
	bFocused = false;
	ApplyState();
}

void UGothamButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bHoveredNow = true;
	// The mouse moves focus too, so there is only ever one "current" item and the highlight follows the pointer.
	if (GetIsFocusable() && !bFocused && GothamUI::HoverMovesFocus())
	{
		SetFocus();
	}
	ApplyState();
}

void UGothamButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bHoveredNow = false;
	ApplyState();
}

void UGothamButton::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPressedNow = true;
	ApplyState();
}

void UGothamButton::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPressedNow = false;
	ApplyState();
}

void UGothamButton::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	ApplyState();
}

void UGothamButton::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	ApplyState();
}

void UGothamButton::NativeOnEnabled()
{
	Super::NativeOnEnabled();
	ApplyState();
}

void UGothamButton::NativeOnDisabled()
{
	Super::NativeOnDisabled();
	ApplyState();
}
