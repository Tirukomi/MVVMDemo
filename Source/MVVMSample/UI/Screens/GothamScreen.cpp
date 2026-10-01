// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/GothamScreen.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/GothamUIInput.h"
#include "Slate/SObjectWidget.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Widgets/GothamAcceptable.h"
#include "UI/Widgets/GothamActionBar.h"
#include "UI/Widgets/GothamButton.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Widgets/GothamMenuList.h"
#include "UI/Widgets/GothamScrim.h"
#include "UI/Widgets/GothamText.h"

UGothamScreen::UGothamScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SlideFrom(-GothamMotion::ScreenSlide, 0.f)
{
	SetIsFocusable(true);
	// Back closes the screen (Common UI's default back handling); screens that must be answered turn it off.
	bIsBackHandler = true;
}

TOptional<FUIInputConfig> UGothamScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UGothamScreen::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { OnPaletteChanged(); });
	OnPaletteChanged();

	// Bindings outlive the Slate widget (a pooled screen constructs again), so each is registered once.
	if (!AcceptLabel.IsEmpty() && !AcceptHandle.IsValid())
	{
		FBindUIActionArgs Args(UGothamUIInputData::Get().GetAcceptAction(), true, FSimpleDelegate::CreateUObject(this, &UGothamScreen::AcceptFocused));
		Args.OverrideDisplayName = AcceptLabel;
		AcceptHandle = RegisterUIActionBinding(Args);
	}
	if (!ToggleActionName.IsNone() && !ToggleHandle.IsValid())
	{
		ToggleHandle = BindAction(FindGameplayAction(ToggleActionName), IE_Pressed, FSimpleDelegate::CreateWeakLambda(this, [this]() { DeactivateWidget(); }));
	}
	// Another screen's key opens that screen on top (gameplay input is blocked in menus, so the screen answers it).
	if (bOpensScreensByKey && ToggleActionName != TEXT("ClueLog") && !ClueLogHandle.IsValid())
	{
		ClueLogHandle = BindAction(FindGameplayAction(TEXT("ClueLog")), IE_Pressed, FSimpleDelegate::CreateWeakLambda(this, [this]()
		{
			if (UGothamUISubsystem* UI = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>() : nullptr)
			{
				UI->ToggleClueLog();
			}
		}));
	}
}

void UGothamScreen::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UGothamScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	// Common UI's router focuses the desired target through the local player's pending Slate operations, which only
	// apply when the viewport next handles input. When a screen opens over another one the HUD is briefly the leaf and
	// focus sits on the game viewport, so the player's first press would go to the game. Focus directly instead.
	if (const UWidget* Target = GetDesiredFocusTarget())
	{
		const TSharedPtr<SWidget> TargetSlate = Target->GetCachedWidget();
		const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		if (TargetSlate.IsValid() && LocalPlayer)
		{
			FSlateApplication::Get().SetUserFocus(LocalPlayer->GetControllerId(), TargetSlate, EFocusCause::SetDirectly);
		}
	}
	if (SlideTarget)
	{
		GothamMotion::SlideIn(SlideTarget, SlideFrom);
	}
}

void UGothamScreen::NativeOnDeactivated()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGothamUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamUISubsystem>() : nullptr;
	if (UI && UI->IsCovered(this))
	{
		NativeOnCovered();
	}
	else
	{
		NativeOnClosed();
	}
	Super::NativeOnDeactivated();
}

UWidget* UGothamScreen::NativeGetDesiredFocusTarget() const
{
	return DefaultFocus ? DefaultFocus.Get() : Super::NativeGetDesiredFocusTarget();
}

const UInputAction* UGothamScreen::FindGameplayAction(FName ActionName) const
{
	const AGothamPlayerController* PC = GetOwningPlayer<AGothamPlayerController>();
	return PC ? PC->FindAction(ActionName) : nullptr;
}

FUIActionBindingHandle UGothamScreen::BindAction(const UInputAction* Action, EInputEvent Event, FSimpleDelegate Handler)
{
	if (!Action)
	{
		return FUIActionBindingHandle();
	}
	FBindUIActionArgs Args(Action, false, MoveTemp(Handler));
	Args.KeyEvent = Event;
	return RegisterUIActionBinding(Args);
}

void UGothamScreen::AcceptFocused()
{
	// The same as pressing Enter on the current item: find the item that has focus and click it. A clicked prompt has
	// already put focus back on the item that was current before the pointer went to the prompt.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const TSharedPtr<SWidget> ScreenSlate = GetCachedWidget();
	for (TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetUserFocusedWidget(LocalPlayer ? LocalPlayer->GetControllerId() : 0);
		Widget.IsValid() && Widget != ScreenSlate; Widget = Widget->GetParentWidget())
	{
		if (Widget->GetType() == TEXT("SObjectWidget"))
		{
			if (IGothamAcceptable* Item = Cast<IGothamAcceptable>(StaticCastSharedPtr<SObjectWidget>(Widget)->GetWidgetObject()))
			{
				Item->Accept();
				return;
			}
		}
	}
}

void UGothamScreen::OnPaletteChanged()
{
	for (int32 i = 0; i < TokenTexts.Num(); ++i)
	{
		if (TokenTexts[i])
		{
			TokenTexts[i]->SetColorAndOpacity(GothamStyle::Token(this, TokenTextColors[i]));
		}
	}
	for (UWidget* Bar : AccentBars)
	{
		if (UBorder* Border = Cast<UBorder>(Bar))
		{
			Border->SetBrushColor(GothamStyle::Token(this, EGothamColorToken::Accent));
		}
	}
	if (UBorder* Rule = Cast<UBorder>(HeaderRule))
	{
		Rule->SetBrushColor(GothamStyle::Token(this, EGothamColorToken::PanelEdge, 0.5f));
	}
}

UVerticalBox* UGothamScreen::BuildMenuFrame(const FText& Section, const FText& Title, float BlurStrength)
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	// The world behind menus: blurred, then darkened more on the left where the column sits.
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	Blur->SetBlurStrength(BlurStrength);
	Blur->SetApplyAlphaToBlur(true);
	Blur->SetPadding(FMargin(0.f));
	Blur->SetHorizontalAlignment(HAlign_Fill);
	Blur->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* BlurSlot = Root->AddChildToOverlay(Blur);
	BlurSlot->SetHorizontalAlignment(HAlign_Fill);
	BlurSlot->SetVerticalAlignment(VAlign_Fill);
	Blur->SetContent(WidgetTree->ConstructWidget<UGothamScrim>());

	UBorder* MarginBox = WidgetTree->ConstructWidget<UBorder>();
	MarginBox->SetBrushColor(FLinearColor::Transparent);
	MarginBox->SetPadding(FMargin(96.f, 64.f, 96.f, 48.f));
	UOverlaySlot* MarginSlot = Root->AddChildToOverlay(MarginBox);
	MarginSlot->SetHorizontalAlignment(HAlign_Fill);
	MarginSlot->SetVerticalAlignment(VAlign_Fill);

	FrameBox = WidgetTree->ConstructWidget<UVerticalBox>();
	MarginBox->SetContent(FrameBox);
	SlideTarget = FrameBox;

	FrameBox->AddChildToVerticalBox(MakeText(Section, EGothamTextStyle::Label, EGothamColorToken::Accent));
	UTextBlock* TitleText = MakeText(Title, EGothamTextStyle::Title, EGothamColorToken::TextPrimary);
	FrameBox->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 2.f, 0.f, 10.f));

	// Header rule: a short accent bar running into a thin full-width line.
	UHorizontalBox* Rule = WidgetTree->ConstructWidget<UHorizontalBox>();
	FrameBox->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 0.f, 0.f, 26.f));
	USizeBox* AccentBox = WidgetTree->ConstructWidget<USizeBox>();
	AccentBox->SetWidthOverride(64.f);
	AccentBox->SetHeightOverride(3.f);
	UBorder* Accent = WidgetTree->ConstructWidget<UBorder>();
	AccentBox->SetContent(Accent);
	AccentBars.Add(Accent);
	Rule->AddChildToHorizontalBox(AccentBox)->SetVerticalAlignment(VAlign_Center);
	USizeBox* LineBox = WidgetTree->ConstructWidget<USizeBox>();
	LineBox->SetHeightOverride(1.f);
	UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
	LineBox->SetContent(Line);
	HeaderRule = Line;
	UHorizontalBoxSlot* LineSlot = Rule->AddChildToHorizontalBox(LineBox);
	LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LineSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	FrameBox->AddChildToVerticalBox(Column)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Column;
}

void UGothamScreen::AddFooter(UWidget* Footer)
{
	if (FrameBox && Footer)
	{
		UVerticalBoxSlot* FooterSlot = FrameBox->AddChildToVerticalBox(Footer);
		FooterSlot->SetHorizontalAlignment(HAlign_Right);
		FooterSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
}

UTextBlock* UGothamScreen::MakeText(const FText& Text, EGothamTextStyle Style, EGothamColorToken Color)
{
	UTextBlock* Block = WidgetTree->ConstructWidget<UGothamText>();
	GothamStyle::ApplyText(Block, Style, GothamStyle::Token(this, Color));
	Block->SetText(Text);
	TokenTexts.Add(Block);
	TokenTextColors.Add(Color);
	return Block;
}

UGothamButton* UGothamScreen::AddMenuItem(UGothamMenuList* List, const FText& Label) const
{
	UGothamButton* Button = WidgetTree->ConstructWidget<UGothamButton>();
	Button->SetKind(EGothamButtonKind::MenuItem);
	Button->SetLabel(Label);
	List->AddItem(Button);
	return Button;
}

UWidget* UGothamScreen::MakeActionBar(const FText& InAcceptLabel, const FText& BackLabel)
{
	// Read when the screen constructs, after it is built: the back handler takes its label then, and so does accept.
	AcceptLabel = InAcceptLabel;
	bIsBackActionDisplayedInActionBar = bIsBackHandler;
	OverrideBackActionDisplayName = BackLabel;
	return WidgetTree->ConstructWidget<UGothamActionBar>();
}
