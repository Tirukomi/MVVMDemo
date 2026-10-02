// Copyright IG. All Rights Reserved.

#include "UI/Screens/PauseMenuScreen.h"

#include "UI/Style/MvsMetrics.h"
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
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsMenuList.h"
#include "UI/Widgets/MvsPanel.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"

#define LOCTEXT_NAMESPACE "Mvs.PauseMenu"

TSharedRef<SWidget> UPauseMenuScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "MVVM Sample"), LOCTEXT("Title", "Paused"));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the menu, a highlight bar sliding behind the current item.
		USizeBox* MenuWidth = WidgetTree->ConstructWidget<USizeBox>();
		MenuWidth->SetMinDesiredWidth(380.f);
		Split->AddChildToHorizontalBox(MenuWidth)->SetVerticalAlignment(VAlign_Top);
		UMvsMenuList* Menu = WidgetTree->ConstructWidget<UMvsMenuList>();
		MenuWidth->SetContent(Menu);

		UMvsButton* Resume = AddMenuItem(Menu, LOCTEXT("Resume", "Resume"));
		UMvsButton* CaseFile = AddMenuItem(Menu, LOCTEXT("CaseFile", "Case file"));
		UMvsButton* Settings = AddMenuItem(Menu, LOCTEXT("Settings", "Settings"));
		UMvsButton* Quit = AddMenuItem(Menu, LOCTEXT("Quit", "Quit"));
		Resume->OnClicked().AddUObject(this, &UPauseMenuScreen::OnResume);
		CaseFile->OnClicked().AddUObject(this, &UPauseMenuScreen::OnCaseFile);
		Settings->OnClicked().AddUObject(this, &UPauseMenuScreen::OnSettings);
		Quit->OnClicked().AddUObject(this, &UPauseMenuScreen::OnQuit);
		DefaultFocus = Resume;

		UHorizontalBoxSlot* Gap = Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>());
		Gap->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Right: where the investigation stands, so pausing doubles as a status check.
		USizeBox* StatusWidth = WidgetTree->ConstructWidget<USizeBox>();
		StatusWidth->SetWidthOverride(MvsMetrics::PauseStatusWidth);
		Split->AddChildToHorizontalBox(StatusWidth)->SetVerticalAlignment(VAlign_Top);
		StatusPanel = WidgetTree->ConstructWidget<UMvsPanel>();
		StatusPanel->SetPanelPadding(MvsMetrics::PauseStatusPadding);
		StatusPanel->SetShape(12.f, EMvsChamfer::Opposite);
		StatusWidth->SetContent(StatusPanel);

		UVerticalBox* Status = WidgetTree->ConstructWidget<UVerticalBox>();
		StatusPanel->SetContent(Status);
		Status->AddChildToVerticalBox(MakeText(LOCTEXT("ObjectiveLabel", "Objective"), EMvsTextStyle::Label, EMvsColorToken::TextMuted));
		ObjectiveText = MakeText(FText::GetEmpty(), EMvsTextStyle::Header, EMvsColorToken::TextPrimary);
		ObjectiveText->SetAutoWrapText(true);
		Status->AddChildToVerticalBox(ObjectiveText)->SetPadding(FMargin(0.f, 2.f, 0.f, 14.f));
		Status->AddChildToVerticalBox(MakeText(LOCTEXT("EvidenceLabel", "Evidence"), EMvsTextStyle::Label, EMvsColorToken::TextMuted));
		EvidenceText = MakeText(FText::GetEmpty(), EMvsTextStyle::Numeric, EMvsColorToken::Accent);
		Status->AddChildToVerticalBox(EvidenceText)->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));

		AddFooter(MakeActionBar(LOCTEXT("Select", "Select"), LOCTEXT("Back", "Resume")));
	}
	return Super::RebuildWidget();
}

void UPauseMenuScreen::ApplyTheme(const FMvsTheme& Theme)
{
	Super::ApplyTheme(Theme);
	if (StatusPanel)
	{
		StatusPanel->SetColors(Theme.Color(EMvsColorToken::Panel, Theme.PanelAlpha()),
			Theme.Color(EMvsColorToken::PanelEdge, 0.7f));
		StatusPanel->SetAccent(Theme.Color(EMvsColorToken::Accent), 3.f);
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
	UMvsViewModelSubsystem* ViewModels = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>() : nullptr;
	if (!ViewModels)
	{
		return;
	}
	Objectives = ViewModels->GetObjectives();
	Clues = ViewModels->GetClues();
	using FClues = UClueListViewModel::FFieldNotificationClassDescriptor;
	MvsMVVM::Bind(Objectives, this, &UPauseMenuScreen::OnStatusChanged, { UObjectivesViewModel::FFieldNotificationClassDescriptor::ObjectiveTitle });
	MvsMVVM::Bind(Clues, this, &UPauseMenuScreen::OnStatusChanged, { FClues::Entries, FClues::DiscoveredCount });
	RefreshStatus();
}

void UPauseMenuScreen::NativeDestruct()
{
	MvsMVVM::Unbind(Objectives, this);
	MvsMVVM::Unbind(Clues, this);
	Super::NativeDestruct();
}

void UPauseMenuScreen::NativeOnClosed()
{
	// Only when pause itself closes: settings and the case file open on top of it and keep the game paused.
	UGameplayStatics::SetGamePaused(this, false);
	Super::NativeOnClosed();
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
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->ClueLogClass);
	}
}

void UPauseMenuScreen::OnSettings()
{
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->SettingsScreenClass);
	}
}

void UPauseMenuScreen::OnQuit()
{
	auto* UI = GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>();
	TSubclassOf<UConfirmModalScreen> ModalClass = UConfirmModalScreen::StaticClass();
	if (UConfirmModalScreen* Modal = UI ? UI->PushScreen<UConfirmModalScreen>(EMvsUILayer::Modal, ModalClass) : nullptr)
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
