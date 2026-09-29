// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/SettingsScreen.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamOptionRow.h"
#include "ViewModels/SettingsViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.SettingsScreen"

TSharedRef<SWidget> USettingsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Dim->SetContent(Column);

		Column->AddChildToVerticalBox(MakeTitle(LOCTEXT("Title", "SETTINGS")));

		DirtyNote = WidgetTree->ConstructWidget<UTextBlock>();
		DirtyNote->SetJustification(ETextJustify::Center);
		DirtyNote->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(DirtyNote)->SetPadding(FMargin(0.f, 4.f, 0.f, 12.f));

		// Rows scroll, so large UI scales and long translations never push the buttons off screen.
		USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetMaxDesiredHeight(380.f);
		Column->AddChildToVerticalBox(Height);
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		Height->SetContent(Scroll);

		const EGothamSetting Order[] = {
			EGothamSetting::Language, EGothamSetting::ColorVision, EGothamSetting::UIScale, EGothamSetting::HighContrast,
			EGothamSetting::ReducedMotion, EGothamSetting::WheelMode, EGothamSetting::SubtitleSize, EGothamSetting::SubtitleBackground };
		for (const EGothamSetting Setting : Order)
		{
			UGothamOptionRow* Row = WidgetTree->ConstructWidget<UGothamOptionRow>();
			Scroll->AddChild(Row);
			Rows.Add(Row);
		}
		DefaultFocus = Rows[0]->GetPrimaryFocusTarget();

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

		auto AddAction = [&](const FText& Label, TFunction<void()> Action)
		{
			UGothamButton* Button = WidgetTree->ConstructWidget<UGothamButton>();
			Button->SetLabel(Label);
			Button->OnClicked().AddLambda(MoveTemp(Action));
			Buttons->AddChildToHorizontalBox(Button)->SetPadding(FMargin(4.f, 0.f));
		};
		AddAction(LOCTEXT("Apply", "Apply"), [this]() { if (ViewModel) { ViewModel->Apply(); } });
		AddAction(LOCTEXT("Revert", "Revert"), [this]() { if (ViewModel) { ViewModel->Revert(); } });
		AddAction(LOCTEXT("Defaults", "Defaults"), [this]() { if (ViewModel) { ViewModel->ResetDefaults(); } });
		AddAction(LOCTEXT("Controls", "Controls"), [this]() { OpenControls(); });
		AddAction(LOCTEXT("Back", "Back"), [this]() { DeactivateWidget(); });

		Column->AddChildToVerticalBox(MakeHintBar(LOCTEXT("Select", "Select"), LOCTEXT("BackHint", "Back (discards unapplied changes)")))
			->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void USettingsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		ViewModel = Settings->GetViewModel();
		const EGothamSetting Order[] = {
			EGothamSetting::Language, EGothamSetting::ColorVision, EGothamSetting::UIScale, EGothamSetting::HighContrast,
			EGothamSetting::ReducedMotion, EGothamSetting::WheelMode, EGothamSetting::SubtitleSize, EGothamSetting::SubtitleBackground };
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			Rows[i]->Setup(Order[i], ViewModel);
		}
		using FVM = USettingsViewModel::FFieldNotificationClassDescriptor;
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsDirty,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &USettingsScreen::OnViewModelChanged));
		ViewModel->AddFieldValueChangedDelegate(FVM::LanguageValue,
			INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &USettingsScreen::OnViewModelChanged));
	}
	RefreshDirtyNote();
}

void USettingsScreen::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	Super::NativeDestruct();
}

void USettingsScreen::NativeOnDeactivated()
{
	// Leaving with unapplied changes discards them, so a previewed language or scale never sticks by accident.
	if (ViewModel)
	{
		ViewModel->Revert();
	}
	Super::NativeOnDeactivated();
}

void USettingsScreen::RefreshDirtyNote()
{
	if (!DirtyNote)
	{
		return;
	}
	const bool bDirty = ViewModel && ViewModel->GetIsDirty();
	DirtyNote->SetText(bDirty ? LOCTEXT("Unapplied", "Unapplied changes. Choose Apply to keep them.") : FText::GetEmpty());
}

void USettingsScreen::OpenControls()
{
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass.LoadSynchronous());
	}
}

#undef LOCTEXT_NAMESPACE
