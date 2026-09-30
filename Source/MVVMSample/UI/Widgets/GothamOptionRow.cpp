// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Style/GothamStyle.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Widgets/GothamSelectorDecor.h"
#include "ViewModels/SettingsViewModel.h"

namespace
{
	constexpr float SelectorWidth = 260.f;
}

UGothamOptionRow::UGothamOptionRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

TSharedRef<SWidget> UGothamOptionRow::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// A transparent border so the whole row (not just its text) takes hover and clicks.
		UBorder* Hit = WidgetTree->ConstructWidget<UBorder>();
		Hit->SetBrushColor(FLinearColor::Transparent);
		Hit->SetPadding(FMargin(22.f, 6.f, 14.f, 6.f));
		WidgetTree->RootWidget = Hit;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Hit->SetContent(Row);

		LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetFont(GothamStyle::Font(EGothamTextStyle::BodyStrong));
		LabelText->SetAutoWrapText(true);
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));

		USizeBox* SelectorBox = WidgetTree->ConstructWidget<USizeBox>();
		SelectorBox->SetWidthOverride(SelectorWidth);
		Row->AddChildToHorizontalBox(SelectorBox)->SetVerticalAlignment(VAlign_Center);

		UOverlay* Selector = WidgetTree->ConstructWidget<UOverlay>();
		SelectorBox->SetContent(Selector);
		Decor = WidgetTree->ConstructWidget<UGothamSelectorDecor>();
		UOverlaySlot* DecorSlot = Selector->AddChildToOverlay(Decor);
		DecorSlot->SetHorizontalAlignment(HAlign_Fill);
		DecorSlot->SetVerticalAlignment(VAlign_Fill);

		ValueText = WidgetTree->ConstructWidget<UTextBlock>();
		ValueText->SetFont(GothamStyle::Font(EGothamTextStyle::Header));
		ValueText->SetJustification(ETextJustify::Center);
		UOverlaySlot* ValueSlot = Selector->AddChildToOverlay(ValueText);
		ValueSlot->SetHorizontalAlignment(HAlign_Center);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		ValueSlot->SetPadding(FMargin(24.f, 2.f, 24.f, 8.f));
	}
	return Super::RebuildWidget();
}

void UGothamOptionRow::Setup(EGothamSetting InSetting, USettingsViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	Setting = InSetting;
	ViewModel = InViewModel;
	if (ViewModel)
	{
		// Any value change (or a language switch, which re-broadcasts every text) refreshes the row.
		using FVM = USettingsViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGothamOptionRow::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::LanguageValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::ColorVisionValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::UIScaleValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::HighContrastValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::ReducedMotionValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::WheelModeValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::ScanModeValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::SubtitleSizeValue, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::SubtitleBackgroundValue, Delegate);
	}
	Refresh();
}

void UGothamOptionRow::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyColors(); });
	ApplyColors();
}

void UGothamOptionRow::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	SettingsListener.Reset();
	Super::NativeDestruct();
}

FNavigationReply UGothamOptionRow::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply)
{
	// Left / right belong to the selector; up / down still move between rows.
	switch (InNavigationEvent.GetNavigationType())
	{
	case EUINavigation::Left:  Step(-1); return FNavigationReply::Stop();
	case EUINavigation::Right: Step(+1); return FNavigationReply::Stop();
	default: return Super::NativeOnNavigation(MyGeometry, InNavigationEvent, InDefaultReply);
	}
}

FReply UGothamOptionRow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		Step(+1);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGothamOptionRow::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	// Left half of the selector steps back, anything to the right of its centre (or the label) steps forward.
	const FGeometry& DecorGeometry = Decor->GetCachedGeometry();
	const float CentreX = DecorGeometry.GetAbsolutePosition().X + DecorGeometry.GetAbsoluteSize().X * 0.5f;
	const float X = InMouseEvent.GetScreenSpacePosition().X;
	const bool bOverSelector = X >= DecorGeometry.GetAbsolutePosition().X;
	Step(bOverSelector && X < CentreX ? -1 : +1);
	return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::Mouse);
}

void UGothamOptionRow::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	if (!bFocused)
	{
		SetFocus();
	}
}

FReply UGothamOptionRow::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	bFocused = true;
	ApplyColors();
	return Super::NativeOnFocusReceived(InGeometry, InFocusEvent);
}

void UGothamOptionRow::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	bFocused = false;
	ApplyColors();
	Super::NativeOnFocusLost(InFocusEvent);
}

void UGothamOptionRow::Step(int32 Direction)
{
	if (ViewModel)
	{
		ViewModel->Cycle(Setting, Direction);
	}
}

void UGothamOptionRow::Refresh()
{
	if (!ViewModel || !LabelText)
	{
		return;
	}
	LabelText->SetText(USettingsViewModel::GetLabel(Setting));
	ValueText->SetText(ViewModel->GetValueText(Setting));
	int32 Index = 0;
	int32 Count = 1;
	ViewModel->GetCurrent().GetOptionPosition(Setting, Index, Count);
	// UI scale clamps at its ends; every other option wraps.
	Decor->SetPosition(Index, Count, Setting != EGothamSetting::UIScale);
}

void UGothamOptionRow::ApplyColors()
{
	if (!LabelText)
	{
		return;
	}
	using namespace GothamStyle;
	const FLinearColor Primary = Token(this, EGothamColorToken::TextPrimary);
	const FLinearColor Muted = Token(this, EGothamColorToken::TextMuted);
	LabelText->SetColorAndOpacity(bFocused ? Primary : Muted);
	ValueText->SetColorAndOpacity(Primary);
	Decor->SetColors(Token(this, EGothamColorToken::Accent), Muted, bFocused);
}
