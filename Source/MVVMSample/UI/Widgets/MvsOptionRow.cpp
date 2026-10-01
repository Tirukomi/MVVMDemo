// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsOptionRow.h"
#include "UI/Style/MvsMetrics.h"
#include "UI/Style/MvsStyle.h"

#include "Accessibility/MvsSettingsListener.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/MvsAccessibility.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Style/MvsLayout.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsSelectorDecor.h"
#include "UI/Widgets/MvsText.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/SettingsViewModel.h"

namespace
{
}

UMvsOptionRow::UMvsOptionRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UMvsButtonStyle::StaticClass();
	SetIsFocusable(true);
}

bool UMvsOptionRow::Initialize()
{
	// Like UMvsButton: Common UI only wires its internal button if the tree has a root when it initializes.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
		// A transparent border so the whole row (not just its text) takes hover and clicks.
		UBorder* Hit = WidgetTree->ConstructWidget<UBorder>();
		Hit->SetBrushColor(FLinearColor::Transparent);
		Hit->SetPadding(MvsMetrics::OptionRowPadding);
		WidgetTree->RootWidget = Hit;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Hit->SetContent(Row);

		LabelText = WidgetTree->ConstructWidget<UMvsText>();
		MvsStyle::SetTextStyle(LabelText, EMvsTextStyle::BodyStrong);
		LabelText->SetAutoWrapText(true);
		MvsLayout::Add(Row, LabelText).Fill().VCenter().Pad(0.f, 0.f, 16.f, 0.f);

		USizeBox* SelectorBox = WidgetTree->ConstructWidget<USizeBox>();
		SelectorBox->SetWidthOverride(MvsMetrics::SelectorWidth);
		MvsLayout::Add(Row, SelectorBox).VCenter();

		UOverlay* Selector = WidgetTree->ConstructWidget<UOverlay>();
		SelectorBox->SetContent(Selector);
		Decor = WidgetTree->ConstructWidget<UMvsSelectorDecor>();
		MvsLayout::Add(Selector, Decor).FillBoth();

		ValueText = WidgetTree->ConstructWidget<UMvsText>();
		MvsStyle::SetTextStyle(ValueText, EMvsTextStyle::Header);
		ValueText->SetJustification(ETextJustify::Center);
		MvsLayout::Add(Selector, ValueText).Center().Pad(24.f, 2.f, 24.f, 8.f);
	}
	return Super::Initialize();
}

void UMvsOptionRow::Setup(EMvsSetting InSetting, USettingsViewModel* InViewModel)
{
	MvsMVVM::Unbind(ViewModel, this);
	Setting = InSetting;
	ViewModel = InViewModel;
	// Any value change, or a language switch, bumps Revision and refreshes the row.
	MvsMVVM::Bind(ViewModel, this, &UMvsOptionRow::OnFieldChanged, { USettingsViewModel::FFieldNotificationClassDescriptor::Revision });
	Refresh();
}

void UMvsOptionRow::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyState(); });
	ApplyState();
	// One sentence for the row, read when it gets focus and again as its value changes: "UI scale: 100%".
	MvsAccessibility::SetText(MvsAccessibility::FindButton(*this), TAttribute<FText>::CreateWeakLambda(this, [this]()
	{
		return ViewModel ? FText::Format(NSLOCTEXT("Mvs.Accessibility", "SettingRow", "{0}: {1}"),
			USettingsViewModel::GetLabel(Setting), ViewModel->GetValueText(Setting)) : FText::GetEmpty();
	}));
}

void UMvsOptionRow::NativeDestruct()
{
	MvsMVVM::Unbind(ViewModel, this);
	SettingsListener.Reset();
	Super::NativeDestruct();
}

FNavigationReply UMvsOptionRow::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply)
{
	// Left / right belong to the selector; up / down still move between rows.
	switch (InNavigationEvent.GetNavigationType())
	{
	case EUINavigation::Left:  Step(-1); return FNavigationReply::Stop();
	case EUINavigation::Right: Step(+1); return FNavigationReply::Stop();
	default: return Super::NativeOnNavigation(MyGeometry, InNavigationEvent, InDefaultReply);
	}
}

FReply UMvsOptionRow::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

void UMvsOptionRow::NativeOnClicked()
{
	Super::NativeOnClicked();
	Step(PointerDirection.Get(+1));
	PointerDirection.Reset();
}

void UMvsOptionRow::NativeOnHovered()
{
	Super::NativeOnHovered();
	// The mouse moves focus too, so there is only ever one "current" row and the highlight follows the pointer.
	if (!bFocused && MvsUI::HoverEnabled())
	{
		SetFocus();
	}
}

void UMvsOptionRow::HandleFocusReceived()
{
	Super::HandleFocusReceived();
	bFocused = true;
	ApplyState();
}

void UMvsOptionRow::HandleFocusLost()
{
	Super::HandleFocusLost();
	bFocused = false;
	ApplyState();
}

void UMvsOptionRow::Step(int32 Direction)
{
	if (ViewModel)
	{
		ViewModel->Cycle(Setting, Direction);
	}
}

void UMvsOptionRow::Refresh()
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
	Decor->SetPosition(Index, Count, Setting != EMvsSetting::UIScale);
}

void UMvsOptionRow::ApplyState()
{
	if (!LabelText)
	{
		return;
	}
	using namespace MvsStyle;
	const FLinearColor Primary = Token(this, EMvsColorToken::TextPrimary);
	const FLinearColor Muted = Token(this, EMvsColorToken::TextMuted);
	LabelText->SetColorAndOpacity(ItemText(this, bFocused));
	ValueText->SetColorAndOpacity(Primary);
	Decor->SetColors(Token(this, EMvsColorToken::Accent), Muted, bFocused);
}
