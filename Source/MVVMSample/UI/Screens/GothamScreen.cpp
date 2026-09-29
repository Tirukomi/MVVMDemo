// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/GothamScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamInputGlyph.h"

UGothamScreen::UGothamScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
}

UWidget* UGothamScreen::NativeGetDesiredFocusTarget() const
{
	return DefaultFocus ? DefaultFocus.Get() : Super::NativeGetDesiredFocusTarget();
}

FReply UGothamScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (bCanDismissWithBack && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UTextBlock* UGothamScreen::MakeTitle(const FText& Text) const
{
	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo Font = Title->GetFont();
	Font.Size = 36;
	Title->SetFont(Font);
	Title->SetText(Text);
	Title->SetJustification(ETextJustify::Center);
	return Title;
}

UGothamButton* UGothamScreen::AddButton(UVerticalBox* Parent, const FText& Label) const
{
	UGothamButton* Button = WidgetTree->ConstructWidget<UGothamButton>();
	Button->SetLabel(Label);
	UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
	ButtonSlot->SetPadding(FMargin(0.f, 6.f));
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	return Button;
}

UHorizontalBox* UGothamScreen::MakeHintBar(const FText& AcceptLabel, const FText& BackLabel) const
{
	UHorizontalBox* Bar = WidgetTree->ConstructWidget<UHorizontalBox>();

	auto AddHint = [&](const FKey& Keyboard, const FKey& Pad, const FText& Label)
	{
		UGothamInputGlyph* Glyph = WidgetTree->ConstructWidget<UGothamInputGlyph>();
		Glyph->SetFixedKeys(Keyboard, Pad);
		Bar->AddChildToHorizontalBox(Glyph)->SetPadding(FMargin(16.f, 0.f, 6.f, 0.f));

		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 14;
		Text->SetFont(Font);
		Text->SetText(Label);
		Bar->AddChildToHorizontalBox(Text)->SetVerticalAlignment(VAlign_Center);
	};
	AddHint(EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom, AcceptLabel);
	AddHint(EKeys::Escape, EKeys::Gamepad_FaceButton_Right, BackLabel);
	return Bar;
}
