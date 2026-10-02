// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsButton.h"

#include "UI/Style/MvsMetrics.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UI/MvsAccessibility.h"
#include "Styling/SlateBrush.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Widgets/MvsPanel.h"
#include "UI/Widgets/MvsText.h"

UMvsButtonStyle::UMvsButtonStyle()
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

UMvsButton::UMvsButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UMvsButtonStyle::StaticClass();
	SetIsFocusable(true);
}

bool UMvsButton::Initialize()
{
	// UCommonButtonBase only wires its internal button (click, focus, style) if the widget tree already has a
	// root when it initializes. There is no designer asset here, so build the content first.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);

		Frame = WidgetTree->ConstructWidget<UMvsPanel>();
		WidgetTree->RootWidget = Frame;

		Label = WidgetTree->ConstructWidget<UMvsText>();
		Label->SetText(PendingLabel);
		Frame->SetContent(Label);
		ApplyKind();
	}
	return Super::Initialize();
}

void UMvsButton::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	// The theme is resolved once per settings change, not on every focus or hover change (second review 13).
	SettingsListener.Bind(this, [this](const FMvsSettingsData& Data) { Theme = FMvsTheme::FromSettings(Data); ApplyState(); });
	Theme = MvsStyle::Theme(this);
	ApplyState();
	// The label as written (not the capitals the style may show), so readers do not spell it out.
	MvsAccessibility::SetText(MvsAccessibility::FindButton(*this), TAttribute<FText>::CreateWeakLambda(this, [this]()
	{
		const UMvsText* Text = Cast<UMvsText>(Label);
		return Text ? Text->GetSourceText() : (Label ? Label->GetText() : FText::GetEmpty());
	}));
}

void UMvsButton::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UMvsButton::SetLabel(const FText& InLabel)
{
	PendingLabel = InLabel;
	if (Label)
	{
		Label->SetText(InLabel);
	}
}

void UMvsButton::SetKind(EMvsButtonKind InKind)
{
	Kind = InKind;
	if (Kind == EMvsButtonKind::Tab)
	{
		SetIsFocusable(false);
	}
	ApplyKind();
	ApplyState();
}

void UMvsButton::ApplyKind()
{
	if (!Frame || !Label)
	{
		return;
	}
	switch (Kind)
	{
	case EMvsButtonKind::MenuItem:
		MvsStyle::SetTextStyle(Label, EMvsTextStyle::Header);
		MvsText::SetUpperCase(Label, true);
		Label->SetJustification(ETextJustify::Left);
		Frame->SetPanelPadding(MvsMetrics::MenuItemPadding);
		break;
	case EMvsButtonKind::Tab:
		MvsStyle::SetTextStyle(Label, EMvsTextStyle::Label);
		MvsText::SetUpperCase(Label, true);
		Label->SetJustification(ETextJustify::Center);
		Frame->SetPanelPadding(MvsMetrics::TabPadding);
		Frame->SetShape(MvsMetrics::TabCorner, EMvsChamfer::Opposite);
		break;
	default:
		MvsStyle::SetTextStyle(Label, EMvsTextStyle::BodyStrong);
		MvsText::SetUpperCase(Label, false);
		Label->SetJustification(ETextJustify::Center);
		Frame->SetPanelPadding(MvsMetrics::StandardButtonPadding);
		Frame->SetShape(MvsMetrics::StandardButtonCorner, EMvsChamfer::Opposite);
		break;
	}
}

void UMvsButton::ApplyState()
{
	if (!Frame || !Label)
	{
		return;
	}
	const bool bHot = bFocused || bHoveredNow;
	const bool bSelectedNow = GetSelected();
	const FLinearColor Accent = Theme.Color(Kind == EMvsButtonKind::Danger ? EMvsColorToken::Danger : EMvsColorToken::Accent);
	const FLinearColor Clear = FLinearColor::Transparent;

	switch (Kind)
	{
	case EMvsButtonKind::MenuItem:
		// Focus is drawn by the list's sliding highlight; the item only brightens its label.
		Frame->SetColors(Clear, Clear, 0.f);
		Frame->SetAccent(Clear, 0.f);
		Frame->SetGlow(Clear, 0.f);
		Label->SetColorAndOpacity(Theme.ItemText(bHot));
		break;

	case EMvsButtonKind::Tab:
		Frame->SetColors(bSelectedNow ? FLinearColor(Accent.R, Accent.G, Accent.B, 0.92f) : Clear,
			bHot && !bSelectedNow ? Theme.Color(EMvsColorToken::PanelEdge, 0.8f) : Clear, 1.f);
		Frame->SetAccent(Clear, 0.f);
		Frame->SetGlow(Clear, 0.f);
		// Dark text on the selected (accent) tab, so selection never relies on colour alone: it is also a filled block.
		Label->SetColorAndOpacity(bSelectedNow ? Theme.Color(EMvsColorToken::Panel)
			: Theme.ItemText(bHot));
		break;

	default:
	{
		const float PanelA = Theme.PanelAlpha();
		FLinearColor Fill = Theme.Color(EMvsColorToken::Panel, PanelA);
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
			: Kind == EMvsButtonKind::Danger ? FLinearColor(Accent.R, Accent.G, Accent.B, 0.55f)
			: Theme.Color(EMvsColorToken::PanelEdge, 0.6f);
		Frame->SetColors(Fill, Edge, bHot ? 1.5f : 1.f);
		Frame->SetAccent(Accent, bHot ? 3.f : 0.f);
		Frame->SetGlow(FLinearColor(Accent.R, Accent.G, Accent.B, 0.3f), bHot ? 6.f : 0.f);
		Label->SetColorAndOpacity(Theme.ItemText(bHot));
		break;
	}
	}
	SetRenderOpacity(GetIsEnabled() ? 1.f : 0.4f);
}

void UMvsButton::HandleFocusReceived()
{
	Super::HandleFocusReceived();
	bFocused = true;
	ApplyState();
}

void UMvsButton::HandleFocusLost()
{
	Super::HandleFocusLost();
	bFocused = false;
	ApplyState();
}

void UMvsButton::NativeOnHovered()
{
	Super::NativeOnHovered();
	bHoveredNow = MvsUI::HoverEnabled();
	// The mouse moves focus too, so there is only ever one "current" item and the highlight follows the pointer.
	if (bHoveredNow && GetIsFocusable() && !bFocused)
	{
		SetFocus();
	}
	ApplyState();
}

void UMvsButton::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bHoveredNow = false;
	ApplyState();
}

void UMvsButton::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPressedNow = true;
	ApplyState();
}

void UMvsButton::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPressedNow = false;
	ApplyState();
}

void UMvsButton::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	ApplyState();
}

void UMvsButton::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	ApplyState();
}

void UMvsButton::NativeOnEnabled()
{
	Super::NativeOnEnabled();
	ApplyState();
}

void UMvsButton::NativeOnDisabled()
{
	Super::NativeOnDisabled();
	ApplyState();
}
