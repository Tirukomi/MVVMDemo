// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Style/GothamStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Widgets/GothamButton.h"
#include "ViewModels/SettingsViewModel.h"

TSharedRef<SWidget> UGothamOptionRow::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		WidgetTree->RootWidget = Row;

		USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>();
		LabelBox->SetWidthOverride(280.f);
		Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);
		LabelText = WidgetTree->ConstructWidget<UTextBlock>();
		LabelText->SetFont(GothamStyle::Font(EGothamTextStyle::BodyStrong));
		LabelText->SetAutoWrapText(true);
		LabelBox->SetContent(LabelText);

		PrevButton = WidgetTree->ConstructWidget<UGothamButton>();
		PrevButton->SetLabel(FText::FromString(TEXT("<")));
		PrevButton->OnClicked().AddLambda([this]() { if (ViewModel) { ViewModel->Cycle(Setting, -1); } });
		Row->AddChildToHorizontalBox(PrevButton)->SetPadding(FMargin(0.f, 2.f));

		USizeBox* ValueBox = WidgetTree->ConstructWidget<USizeBox>();
		ValueBox->SetWidthOverride(240.f);
		Row->AddChildToHorizontalBox(ValueBox)->SetVerticalAlignment(VAlign_Center);
		ValueText = WidgetTree->ConstructWidget<UTextBlock>();
		ValueText->SetFont(GothamStyle::Font(EGothamTextStyle::Header));
		ValueText->SetJustification(ETextJustify::Center);
		ValueBox->SetContent(ValueText);

		NextButton = WidgetTree->ConstructWidget<UGothamButton>();
		NextButton->SetLabel(FText::FromString(TEXT(">")));
		NextButton->OnClicked().AddLambda([this]() { if (ViewModel) { ViewModel->Cycle(Setting, +1); } });
		Row->AddChildToHorizontalBox(NextButton)->SetPadding(FMargin(0.f, 2.f));
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

UWidget* UGothamOptionRow::GetPrimaryFocusTarget() const
{
	return NextButton;
}

void UGothamOptionRow::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
}

void UGothamOptionRow::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void UGothamOptionRow::Refresh()
{
	if (!ViewModel || !LabelText)
	{
		return;
	}
	LabelText->SetText(USettingsViewModel::GetLabel(Setting));
	ValueText->SetText(ViewModel->GetValueText(Setting));
}
