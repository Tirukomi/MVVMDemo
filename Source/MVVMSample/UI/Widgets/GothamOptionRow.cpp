// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Style/GothamStyle.h"

#include "Accessibility/GothamSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Style/GothamLayout.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamSelectorDecor.h"
#include "UI/Widgets/GothamText.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/SettingsViewModel.h"

namespace
{
	constexpr float SelectorWidth = 260.f;
}

UGothamOptionRow::UGothamOptionRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UGothamButtonStyle::StaticClass();
	SetIsFocusable(true);
}

bool UGothamOptionRow::Initialize()
{
	// Like UGothamButton: Common UI only wires its internal button if the tree has a root when it initializes.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		// A transparent border so the whole row (not just its text) takes hover and clicks.
		UBorder* Hit = WidgetTree->ConstructWidget<UBorder>();
		Hit->SetBrushColor(FLinearColor::Transparent);
		Hit->SetPadding(FMargin(22.f, 6.f, 14.f, 6.f));
		WidgetTree->RootWidget = Hit;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Hit->SetContent(Row);

		LabelText = WidgetTree->ConstructWidget<UGothamText>();
		LabelText->SetFont(GothamStyle::Font(EGothamTextStyle::BodyStrong));
		LabelText->SetAutoWrapText(true);
		GothamLayout::Add(Row, LabelText).Fill().VCenter().Pad(0.f, 0.f, 16.f, 0.f);

		USizeBox* SelectorBox = WidgetTree->ConstructWidget<USizeBox>();
		SelectorBox->SetWidthOverride(SelectorWidth);
		GothamLayout::Add(Row, SelectorBox).VCenter();

		UOverlay* Selector = WidgetTree->ConstructWidget<UOverlay>();
		SelectorBox->SetContent(Selector);
		Decor = WidgetTree->ConstructWidget<UGothamSelectorDecor>();
		GothamLayout::Add(Selector, Decor).FillBoth();

		ValueText = WidgetTree->ConstructWidget<UGothamText>();
		ValueText->SetFont(GothamStyle::Font(EGothamTextStyle::Header));
		ValueText->SetJustification(ETextJustify::Center);
		GothamLayout::Add(Selector, ValueText).Center().Pad(24.f, 2.f, 24.f, 8.f);
	}
	return Super::Initialize();
}

void UGothamOptionRow::Setup(EGothamSetting InSetting, USettingsViewModel* InViewModel)
{
	GothamMVVM::Unbind(ViewModel, this);
	Setting = InSetting;
	ViewModel = InViewModel;
	// Any value change, or a language switch, bumps Revision and refreshes the row.
	GothamMVVM::Bind(ViewModel, this, &UGothamOptionRow::OnFieldChanged, { USettingsViewModel::FFieldNotificationClassDescriptor::Revision });
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
	GothamMVVM::Unbind(ViewModel, this);
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

FReply UGothamOptionRow::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// The internal button takes the press itself, so read where it lands on the way down (preview) and let it pass:
	// the left half of the selector steps back, anything to the right of its centre (or the label) steps forward.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Decor)
	{
		const FGeometry& DecorGeometry = Decor->GetCachedGeometry();
		const float CentreX = DecorGeometry.GetAbsolutePosition().X + DecorGeometry.GetAbsoluteSize().X * 0.5f;
		const float X = InMouseEvent.GetScreenSpacePosition().X;
		const bool bOverSelector = X >= DecorGeometry.GetAbsolutePosition().X;
		PointerDirection = bOverSelector && X < CentreX ? -1 : +1;
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UGothamOptionRow::NativeOnClicked()
{
	Super::NativeOnClicked();
	Step(PointerDirection.Get(+1));
	PointerDirection.Reset();
}

void UGothamOptionRow::NativeOnHovered()
{
	Super::NativeOnHovered();
	// The mouse moves focus too, so there is only ever one "current" row and the highlight follows the pointer.
	if (!bFocused && GothamUI::HoverEnabled())
	{
		SetFocus();
	}
}

void UGothamOptionRow::HandleFocusReceived()
{
	Super::HandleFocusReceived();
	bFocused = true;
	ApplyColors();
}

void UGothamOptionRow::HandleFocusLost()
{
	Super::HandleFocusLost();
	bFocused = false;
	ApplyColors();
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
	LabelText->SetColorAndOpacity(ItemText(this, bFocused));
	ValueText->SetColorAndOpacity(Primary);
	Decor->SetColors(Token(this, EGothamColorToken::Accent), Muted, bFocused);
}
