// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/PauseMenuScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamMenuList.h"
#include "UI/Widgets/GothamPanel.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/GothamViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.PauseMenu"

TSharedRef<SWidget> UPauseMenuScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		// The key that opened this screen (Pause) closes it again.
		ToggleActionName = TEXT("Pause");
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Blackwater Ops"), LOCTEXT("Title", "Paused"));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the menu, a highlight bar sliding behind the current item.
		USizeBox* MenuWidth = WidgetTree->ConstructWidget<USizeBox>();
		MenuWidth->SetMinDesiredWidth(380.f);
		Split->AddChildToHorizontalBox(MenuWidth)->SetVerticalAlignment(VAlign_Top);
		UGothamMenuList* Menu = WidgetTree->ConstructWidget<UGothamMenuList>();
		MenuWidth->SetContent(Menu);

		UGothamButton* Resume = AddMenuItem(Menu, LOCTEXT("Resume", "Resume"));
		UGothamButton* CaseFile = AddMenuItem(Menu, LOCTEXT("CaseFile", "Case file"));
		UGothamButton* Settings = AddMenuItem(Menu, LOCTEXT("Settings", "Settings"));
		UGothamButton* Quit = AddMenuItem(Menu, LOCTEXT("Quit", "Quit"));
		Resume->OnClicked().AddUObject(this, &UPauseMenuScreen::OnResume);
		CaseFile->OnClicked().AddUObject(this, &UPauseMenuScreen::OnCaseFile);
		Settings->OnClicked().AddUObject(this, &UPauseMenuScreen::OnSettings);
		Quit->OnClicked().AddUObject(this, &UPauseMenuScreen::OnQuit);
		DefaultFocus = Resume;

		UHorizontalBoxSlot* Gap = Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>());
		Gap->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Right: where the investigation stands, so pausing doubles as a status check.
		USizeBox* StatusWidth = WidgetTree->ConstructWidget<USizeBox>();
		StatusWidth->SetWidthOverride(360.f);
		Split->AddChildToHorizontalBox(StatusWidth)->SetVerticalAlignment(VAlign_Top);
		StatusPanel = WidgetTree->ConstructWidget<UGothamPanel>();
		StatusPanel->SetPanelPadding(FMargin(22.f, 16.f, 22.f, 18.f));
		StatusPanel->SetShape(12.f, EGothamChamfer::Opposite);
		StatusWidth->SetContent(StatusPanel);

		UVerticalBox* Status = WidgetTree->ConstructWidget<UVerticalBox>();
		StatusPanel->SetContent(Status);
		Status->AddChildToVerticalBox(MakeText(LOCTEXT("ObjectiveLabel", "Objective"), EGothamTextStyle::Label, EGothamColorToken::TextMuted));
		ObjectiveText = MakeText(FText::GetEmpty(), EGothamTextStyle::Header, EGothamColorToken::TextPrimary);
		ObjectiveText->SetAutoWrapText(true);
		Status->AddChildToVerticalBox(ObjectiveText)->SetPadding(FMargin(0.f, 2.f, 0.f, 14.f));
		Status->AddChildToVerticalBox(MakeText(LOCTEXT("EvidenceLabel", "Evidence"), EGothamTextStyle::Label, EGothamColorToken::TextMuted));
		EvidenceText = MakeText(FText::GetEmpty(), EGothamTextStyle::Numeric, EGothamColorToken::Accent);
		Status->AddChildToVerticalBox(EvidenceText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

		AddFooter(MakeHintBar(LOCTEXT("Select", "Select"), LOCTEXT("Back", "Resume")));
	}
	return Super::RebuildWidget();
}

void UPauseMenuScreen::OnPaletteChanged()
{
	Super::OnPaletteChanged();
	if (StatusPanel)
	{
		StatusPanel->SetColors(GothamStyle::Token(this, EGothamColorToken::Panel, GothamStyle::PanelAlpha(this)),
			GothamStyle::Token(this, EGothamColorToken::PanelEdge, 0.7f));
		StatusPanel->SetAccent(GothamStyle::Token(this, EGothamColorToken::Accent), 3.f);
	}
}

void UPauseMenuScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	UGameplayStatics::SetGamePaused(this, true);
	RefreshStatus();
}

void UPauseMenuScreen::NativeConstruct()
{
	Super::NativeConstruct();
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UGothamViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr;
	if (!ViewModels)
	{
		return;
	}
	Objectives = ViewModels->GetObjectives();
	Clues = ViewModels->GetClues();
	using FClues = UClueListViewModel::FFieldNotificationClassDescriptor;
	GothamMVVM::Bind(Objectives, this, &UPauseMenuScreen::OnStatusChanged, { UObjectivesViewModel::FFieldNotificationClassDescriptor::ObjectiveTitle });
	GothamMVVM::Bind(Clues, this, &UPauseMenuScreen::OnStatusChanged, { FClues::Entries, FClues::DiscoveredCount });
	RefreshStatus();
}

void UPauseMenuScreen::NativeDestruct()
{
	GothamMVVM::Unbind(Objectives, this);
	GothamMVVM::Unbind(Clues, this);
	Super::NativeDestruct();
}

void UPauseMenuScreen::NativeOnDeactivated()
{
	UGameplayStatics::SetGamePaused(this, false);
	Super::NativeOnDeactivated();
}

void UPauseMenuScreen::RefreshStatus()
{
	if (!ObjectiveText)
	{
		return;
	}
	ObjectiveText->SetText(Objectives ? Objectives->GetObjectiveTitle() : FText::GetEmpty());
	EvidenceText->SetText(Clues ? FText::Format(LOCTEXT("EvidenceFmt", "{0} / {1}"),
		FText::AsNumber(Clues->GetDiscoveredCount()), FText::AsNumber(Clues->GetTotalCount())) : FText::GetEmpty());
}

void UPauseMenuScreen::OnResume()
{
	DeactivateWidget();
}

void UPauseMenuScreen::OnCaseFile()
{
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ClueLogClass.LoadSynchronous());
	}
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
			FOnConfirmResult::CreateUObject(this, &UPauseMenuScreen::OnQuitConfirmed), /*bDestructive*/ true);
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
