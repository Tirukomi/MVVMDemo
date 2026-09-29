// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Style/GothamStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Widgets/GothamButton.h"

#define LOCTEXT_NAMESPACE "Gotham.ConfirmModal"

TSharedRef<SWidget> UConfirmModalScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.5f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(480.f);
		Dim->SetContent(Width);

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetBrushColor(FLinearColor(0.04f, 0.04f, 0.05f, 0.95f));
		Panel->SetPadding(FMargin(32.f));
		Width->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		TitleText = MakeTitle(FText::GetEmpty());
		Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

		BodyText = WidgetTree->ConstructWidget<UTextBlock>();
		BodyText->SetFont(GothamStyle::Font(EGothamTextStyle::Body));
		BodyText->SetJustification(ETextJustify::Center);
		BodyText->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons);

		UGothamButton* Yes = WidgetTree->ConstructWidget<UGothamButton>();
		Yes->SetLabel(LOCTEXT("Yes", "Yes"));
		Yes->OnClicked().AddLambda([this]() { Finish(true); });
		Buttons->AddChildToHorizontalBox(Yes)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

		UGothamButton* No = WidgetTree->ConstructWidget<UGothamButton>();
		No->SetLabel(LOCTEXT("No", "No"));
		No->OnClicked().AddLambda([this]() { Finish(false); });
		Buttons->AddChildToHorizontalBox(No)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));

		// Safe default: focus lands on "No".
		DefaultFocus = No;
	}
	return Super::RebuildWidget();
}

void UConfirmModalScreen::Setup(const FText& InTitle, const FText& InBody, FOnConfirmResult InCallback)
{
	Callback = MoveTemp(InCallback);
	if (TitleText)
	{
		TitleText->SetText(InTitle);
		BodyText->SetText(InBody);
	}
}

void UConfirmModalScreen::Finish(bool bConfirmed)
{
	if (!bAnswered)
	{
		bAnswered = true;
		Callback.ExecuteIfBound(bConfirmed);
	}
	DeactivateWidget();
}

void UConfirmModalScreen::NativeOnDeactivated()
{
	// Back / Esc counts as "No" so callers always get exactly one answer.
	if (!bAnswered)
	{
		bAnswered = true;
		Callback.ExecuteIfBound(false);
	}
	Super::NativeOnDeactivated();
}

#undef LOCTEXT_NAMESPACE
