// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/GothamScreen.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Containers/Ticker.h"
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
#include "UI/GothamWidgetTick.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamHintButton.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/Widgets/GothamMenuList.h"
#include "UI/Widgets/GothamScrim.h"

UGothamScreen::UGothamScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SlideFrom(-GothamMotion::ScreenSlide, 0.f)
{
	SetIsFocusable(true);
}

TOptional<FUIInputConfig> UGothamScreen::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UGothamScreen::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		PaletteHandle = Settings->OnSettingsChanged.AddWeakLambda(this, [this](const FGothamSettingsData&) { OnPaletteChanged(); });
	}
	OnPaletteChanged();
}

void UGothamScreen::NativeDestruct()
{
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		Settings->OnSettingsChanged.Remove(PaletteHandle);
	}
	Super::NativeDestruct();
}

void UGothamScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (SlideTarget)
	{
		GothamMotion::SlideIn(SlideTarget, SlideFrom);
	}
}

UWidget* UGothamScreen::NativeGetDesiredFocusTarget() const
{
	return DefaultFocus ? DefaultFocus.Get() : Super::NativeGetDesiredFocusTarget();
}

FReply UGothamScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	// The key that opened the screen closes it (J for the case file, Start for pause), however it is bound.
	if (!ToggleActionName.IsNone() && !InKeyEvent.IsRepeat() && IsKeyBoundToAction(Key, ToggleActionName))
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	if (bCanDismissWithBack && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		HandleBack();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGothamScreen::HandleBack()
{
	if (bCanDismissWithBack)
	{
		DeactivateWidget();
	}
}

void UGothamScreen::HandleAccept(TSharedPtr<SWidget> Target)
{
	// Exactly what pressing Enter does, on the item that was current before the pointer went to the prompt (a
	// button, an option row, a binding slot). Clicking the prompt let Slate move focus to the screen, so focus goes
	// back first; the key is sent next frame, outside the prompt's own mouse handling.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const uint32 SlateUser = LocalPlayer ? LocalPlayer->GetControllerId() : 0;
	const TWeakPtr<SWidget> WeakTarget = Target;
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [SlateUser, WeakTarget](float)
	{
		FSlateApplication& App = FSlateApplication::Get();
		if (const TSharedPtr<SWidget> Restore = WeakTarget.Pin())
		{
			App.SetUserFocus(SlateUser, Restore, EFocusCause::SetDirectly);
		}
		App.ProcessKeyDownEvent(FKeyEvent(EKeys::Enter, App.GetModifierKeys(), SlateUser, false, 0, 0));
		App.ProcessKeyUpEvent(FKeyEvent(EKeys::Enter, App.GetModifierKeys(), SlateUser, false, 0, 0));
		return false;
	}));
}

bool UGothamScreen::IsKeyBoundToAction(const FKey& Key, FName ActionName) const
{
	// Not QueryKeysMappedToAction: that only sees active mapping contexts, and the gameplay context is removed while
	// a menu is open. The key profile holds every mappable action's current keys (rebinds included) regardless.
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	const UEnhancedInputUserSettings* UserSettings = Input ? Input->GetUserSettings() : nullptr;
	const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
	const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(ActionName) : nullptr;
	if (!Row)
	{
		return false;
	}
	for (const FPlayerKeyMapping& Mapping : Row->Mappings)
	{
		if (Mapping.GetCurrentKey() == Key)
		{
			return true;
		}
	}
	return false;
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
	UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
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

UHorizontalBox* UGothamScreen::MakeHintBar(const FText& AcceptLabel, const FText& BackLabel)
{
	UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>();
	UGothamHintButton* Accept = AddHint(Bar, EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom, AcceptLabel);
	const TWeakObjectPtr<UGothamHintButton> WeakAccept(Accept);
	Accept->OnClicked().AddWeakLambda(this, [this, WeakAccept]() { HandleAccept(WeakAccept.IsValid() ? WeakAccept->GetFocusBeforePointer() : nullptr); });
	AddHint(Bar, EKeys::Escape, EKeys::Gamepad_FaceButton_Right, BackLabel)->OnClicked().AddUObject(this, &UGothamScreen::HandleBack);
	return Bar;
}

UGothamHintButton* UGothamScreen::AddHint(UHorizontalBox* Bar, const FKey& Keyboard, const FKey& Pad, const FText& Label)
{
	UGothamHintButton* Hint = WidgetTree->ConstructWidget<UGothamHintButton>();
	Hint->SetHint(Keyboard, Pad, Label);
	Bar->AddChildToHorizontalBox(Hint)->SetPadding(FMargin(20.f, 0.f, 0.f, 0.f));
	return Hint;
}
