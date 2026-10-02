// Copyright IG. All Rights Reserved.

#include "UI/Screens/MvsScreen.h"

#include "UI/Style/MvsMetrics.h"
#include "Accessibility/MvsSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SafeZone.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"
#include "Input/MvsUIInput.h"
#include "Slate/SObjectWidget.h"
#include "UI/MvsUISettings.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Widgets/MvsAcceptable.h"
#include "UI/Widgets/MvsActionBar.h"
#include "UI/Widgets/MvsButton.h"
#include "Input/MvsActionSource.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Widgets/MvsMenuList.h"
#include "UI/Widgets/MvsScrim.h"
#include "UI/Widgets/MvsSwatch.h"
#include "UI/Widgets/MvsText.h"

UMvsScreen::UMvsScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SlideFrom(-MvsMotion::ScreenSlide, 0.f)
{
	SetIsFocusable(true);
	// Back closes the screen (Common UI's default back handling); screens that must be answered turn it off.
	bIsBackHandler = true;
}

TOptional<FUIInputConfig> UMvsScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UMvsScreen::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData& Data) { ApplyTheme(FMvsTheme::FromSettings(Data)); });
	ApplyTheme(MvsStyle::Theme(this));

	// Bindings outlive the Slate widget (a pooled screen constructs again), so each is registered once.
	if (!AcceptLabel.IsEmpty() && !AcceptHandle.IsValid())
	{
		FBindUIActionArgs Args(UMvsUIInputData::Get().GetAcceptAction(), true, FSimpleDelegate::CreateUObject(this, &UMvsScreen::AcceptFocused));
		Args.OverrideDisplayName = AcceptLabel;
		AcceptHandle = RegisterUIActionBinding(Args);
	}
	if (!bShortcutsBound)
	{
		bShortcutsBound = true;
		const FMvsScreenShortcut* Own = GetDefault<UMvsUISettings>()->FindShortcutFor(this);
		// The key that opened this screen closes it.
		if (Own && Own->Kind != EMvsShortcutKind::Hold)
		{
			ShortcutHandles.Add(BindAction(FindGameplayAction(Own->Action), IE_Pressed, FSimpleDelegate::CreateWeakLambda(this, [this]() { DeactivateWidget(); })));
		}
		// Another screen's toggle key opens that screen on top (gameplay input is blocked in menus, so the screen answers it).
		for (const FMvsScreenShortcut& Shortcut : UMvsUISettings::GetShortcuts())
		{
			if (bOpensScreensByKey && Shortcut.Kind == EMvsShortcutKind::Toggle && &Shortcut != Own)
			{
				ShortcutHandles.Add(BindAction(FindGameplayAction(Shortcut.Action), IE_Pressed, FSimpleDelegate::CreateWeakLambda(this, [this, Action = Shortcut.Action]()
				{
					if (UMvsUISubsystem* UI = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>() : nullptr)
					{
						UI->HandleShortcut(Action);
					}
				})));
			}
		}
	}
}

FName UMvsScreen::GetShortcutAction() const
{
	const FMvsScreenShortcut* Own = GetDefault<UMvsUISettings>()->FindShortcutFor(this);
	return Own ? Own->Action : NAME_None;
}

void UMvsScreen::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

void UMvsScreen::NativeOnActivated()
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
		MvsMotion::SlideIn(SlideTarget, SlideFrom);
	}
}

void UMvsScreen::NativeOnDeactivated()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UMvsUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsUISubsystem>() : nullptr;
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

UWidget* UMvsScreen::NativeGetDesiredFocusTarget() const
{
	return DefaultFocus ? DefaultFocus.Get() : Super::NativeGetDesiredFocusTarget();
}

const UInputAction* UMvsScreen::FindGameplayAction(FName ActionName) const
{
	return IMvsActionSource::Find(GetOwningPlayer(), ActionName);
}

FUIActionBindingHandle UMvsScreen::BindAction(const UInputAction* Action, EInputEvent Event, FSimpleDelegate Handler)
{
	if (!Action)
	{
		return FUIActionBindingHandle();
	}
	FBindUIActionArgs Args(Action, false, MoveTemp(Handler));
	Args.KeyEvent = Event;
	return RegisterUIActionBinding(Args);
}

void UMvsScreen::AcceptFocused()
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
			if (IMvsAcceptable* Item = Cast<IMvsAcceptable>(StaticCastSharedPtr<SObjectWidget>(Widget)->GetWidgetObject()))
			{
				Item->Accept();
				return;
			}
		}
	}
}

UVerticalBox* UMvsScreen::BuildMenuFrame(const FText& Section, const FText& Title, float BlurStrength)
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	// The world behind menus: blurred, then darkened more on the left where the column sits.
	UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
	SetBackdrop(Blur, BlurStrength);
	Blur->SetApplyAlphaToBlur(true);
	Blur->SetPadding(FMargin(0.f));
	Blur->SetHorizontalAlignment(HAlign_Fill);
	Blur->SetVerticalAlignment(VAlign_Fill);
	UOverlaySlot* BlurSlot = Root->AddChildToOverlay(Blur);
	BlurSlot->SetHorizontalAlignment(HAlign_Fill);
	BlurSlot->SetVerticalAlignment(VAlign_Fill);
	Blur->SetContent(WidgetTree->ConstructWidget<UMvsScrim>());

	// The menu column sits inside the platform's safe zone; the blurred world behind it stays full screen.
	USafeZone* Safe = WidgetTree->ConstructWidget<USafeZone>();
	UOverlaySlot* SafeSlot = Root->AddChildToOverlay(Safe);
	SafeSlot->SetHorizontalAlignment(HAlign_Fill);
	SafeSlot->SetVerticalAlignment(VAlign_Fill);
	UBorder* MarginBox = WidgetTree->ConstructWidget<UBorder>();
	MarginBox->SetBrushColor(FLinearColor::Transparent);
	MarginBox->SetPadding(MvsMetrics::FrameMargin);
	Safe->SetContent(MarginBox);

	FrameBox = WidgetTree->ConstructWidget<UVerticalBox>();
	MarginBox->SetContent(FrameBox);
	SlideTarget = FrameBox;

	FrameBox->AddChildToVerticalBox(MakeText(Section, EMvsTextStyle::Label, EMvsColorToken::Accent));
	UTextBlock* TitleText = MakeText(Title, EMvsTextStyle::Title, EMvsColorToken::TextPrimary);
	FrameBox->AddChildToVerticalBox(TitleText)->SetPadding(MvsMetrics::TitlePadding);

	// Header rule: a short accent bar running into a thin full-width line.
	UHorizontalBox* Rule = WidgetTree->ConstructWidget<UHorizontalBox>();
	FrameBox->AddChildToVerticalBox(Rule)->SetPadding(FMargin(0.f, 0.f, 0.f, MvsMetrics::HeaderRuleGap));
	USizeBox* AccentBox = WidgetTree->ConstructWidget<USizeBox>();
	AccentBox->SetWidthOverride(MvsMetrics::AccentBarWidth);
	AccentBox->SetHeightOverride(MvsMetrics::AccentBarHeight);
	UMvsSwatch* Accent = WidgetTree->ConstructWidget<UMvsSwatch>();
	Accent->SetColorToken(EMvsColorToken::Accent);
	AccentBox->SetContent(Accent);
	Rule->AddChildToHorizontalBox(AccentBox)->SetVerticalAlignment(VAlign_Center);
	USizeBox* LineBox = WidgetTree->ConstructWidget<USizeBox>();
	LineBox->SetHeightOverride(1.f);
	UMvsSwatch* Line = WidgetTree->ConstructWidget<UMvsSwatch>();
	Line->SetColorToken(EMvsColorToken::PanelEdge, 0.5f);
	LineBox->SetContent(Line);
	UHorizontalBoxSlot* LineSlot = Rule->AddChildToHorizontalBox(LineBox);
	LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	LineSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	FrameBox->AddChildToVerticalBox(Column)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Column;
}

void UMvsScreen::SetBackdrop(UBackgroundBlur* Blur, float Strength)
{
	Backdrop = Blur;
	BackdropStrength = Strength;
	if (Backdrop)
	{
		Backdrop->SetBlurStrength(Strength);
	}
}

bool UMvsScreen::IsBackdropBlurEnabled() const
{
	return Backdrop && Backdrop->GetBlurStrength() > 0.f;
}

void UMvsScreen::SetBackdropBlurEnabled(bool bEnabled)
{
	// Strength 0 skips the blur pass entirely; the dim (the blur's content) stays.
	const float Strength = bEnabled ? BackdropStrength : 0.f;
	if (Backdrop && Backdrop->GetBlurStrength() != Strength)
	{
		Backdrop->SetBlurStrength(Strength);
	}
}

void UMvsScreen::AddFooter(UWidget* Footer)
{
	if (FrameBox && Footer)
	{
		UVerticalBoxSlot* FooterSlot = FrameBox->AddChildToVerticalBox(Footer);
		FooterSlot->SetHorizontalAlignment(HAlign_Right);
		FooterSlot->SetPadding(FMargin(0.f, MvsMetrics::FooterGap, 0.f, 0.f));
	}
}

UTextBlock* UMvsScreen::MakeText(const FText& Text, EMvsTextStyle Style, EMvsColorToken Color)
{
	UMvsText* Block = WidgetTree->ConstructWidget<UMvsText>();
	MvsStyle::ApplyText(Block, Style, MvsStyle::Token(this, Color));
	Block->SetColorToken(Color);
	Block->SetText(Text);
	return Block;
}

UMvsButton* UMvsScreen::AddMenuItem(UMvsMenuList* List, const FText& Label) const
{
	UMvsButton* Button = WidgetTree->ConstructWidget<UMvsButton>();
	Button->SetKind(EMvsButtonKind::MenuItem);
	Button->SetLabel(Label);
	List->AddItem(Button);
	return Button;
}

UWidget* UMvsScreen::MakeActionBar(const FText& InAcceptLabel, const FText& BackLabel)
{
	// Read when the screen constructs, after it is built: the back handler takes its label then, and so does accept.
	AcceptLabel = InAcceptLabel;
	bIsBackActionDisplayedInActionBar = bIsBackHandler;
	OverrideBackActionDisplayName = BackLabel;
	return WidgetTree->ConstructWidget<UMvsActionBar>();
}
