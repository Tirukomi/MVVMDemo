// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/SettingsScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Widgets/GothamButton.h"

#define LOCTEXT_NAMESPACE "Gotham.Settings"

TSharedRef<SWidget> USettingsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.75f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Dim->SetContent(Column);
		Column->AddChildToVerticalBox(MakeTitle(LOCTEXT("Title", "SETTINGS")))->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

		UTextBlock* Note = WidgetTree->ConstructWidget<UTextBlock>();
		Note->SetText(LOCTEXT("Note", "Input, accessibility and language options arrive in a later milestone."));
		Column->AddChildToVerticalBox(Note)->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

		UGothamButton* Back = AddButton(Column, LOCTEXT("Back", "Back"));
		Back->OnClicked().AddLambda([this]() { DeactivateWidget(); });
		DefaultFocus = Back;

		Column->AddChildToVerticalBox(MakeHintBar(LOCTEXT("Select", "Select"), LOCTEXT("BackHint", "Back")))
			->SetPadding(FMargin(0.f, 24.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

#undef LOCTEXT_NAMESPACE
