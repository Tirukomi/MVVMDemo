// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/PauseMenuScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Widgets/GothamButton.h"

#define LOCTEXT_NAMESPACE "Gotham.PauseMenu"

TSharedRef<SWidget> UPauseMenuScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// Full-screen dim, menu column centred.
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(360.f);
		Dim->SetContent(Width);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Width->SetContent(Column);

		Column->AddChildToVerticalBox(MakeTitle(LOCTEXT("Title", "PAUSED")))->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

		UGothamButton* Resume = AddButton(Column, LOCTEXT("Resume", "Resume"));
		UGothamButton* Settings = AddButton(Column, LOCTEXT("Settings", "Settings"));
		UGothamButton* Quit = AddButton(Column, LOCTEXT("Quit", "Quit"));
		Resume->OnClicked().AddUObject(this, &UPauseMenuScreen::OnResume);
		Settings->OnClicked().AddUObject(this, &UPauseMenuScreen::OnSettings);
		Quit->OnClicked().AddUObject(this, &UPauseMenuScreen::OnQuit);
		DefaultFocus = Resume;

		Column->AddChildToVerticalBox(MakeHintBar(LOCTEXT("Select", "Select"), LOCTEXT("Back", "Back")))
			->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

void UPauseMenuScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	UGameplayStatics::SetGamePaused(this, true);
}

void UPauseMenuScreen::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);
	Super::NativeOnDeactivated();
}

void UPauseMenuScreen::OnResume()
{
	DeactivateWidget();
}

void UPauseMenuScreen::OnSettings()
{
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass.LoadSynchronous());
	}
}

void UPauseMenuScreen::OnQuit()
{
	auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>();
	TSubclassOf<UConfirmModalScreen> ModalClass = UConfirmModalScreen::StaticClass();
	if (UConfirmModalScreen* Modal = UI ? UI->PushScreen<UConfirmModalScreen>(EGothamUILayer::Modal, ModalClass) : nullptr)
	{
		Modal->Setup(LOCTEXT("QuitTitle", "Quit game?"), LOCTEXT("QuitBody", "Unsaved progress will be lost."),
			FOnConfirmResult::CreateUObject(this, &UPauseMenuScreen::OnQuitConfirmed));
	}
}

void UPauseMenuScreen::OnQuitConfirmed(bool bConfirmed)
{
	if (bConfirmed)
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}

#undef LOCTEXT_NAMESPACE
