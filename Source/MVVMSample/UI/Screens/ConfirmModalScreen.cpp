// Copyright IG. All Rights Reserved.

#include "UI/Screens/ConfirmModalScreen.h"

#include "UI/Style/MvsMetrics.h"
#include "Blueprint/WidgetTree.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsPanel.h"

#define LOCTEXT_NAMESPACE "Mvs.ConfirmModal"

TSharedRef<SWidget> UConfirmModalScreen::RebuildWidget()
{
	// A confirmation must be answered first: no other screen opens underneath it.
	bOpensScreensByKey = false;
	if (!WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = Root;

		// A lighter blur and dim than full menus: the screen underneath stays recognisable.
		UBackgroundBlur* Blur = WidgetTree->ConstructWidget<UBackgroundBlur>();
		SetBackdrop(Blur, 6.f);
		UOverlaySlot* BlurSlot = Root->AddChildToOverlay(Blur);
		BlurSlot->SetHorizontalAlignment(HAlign_Fill);
		BlurSlot->SetVerticalAlignment(VAlign_Fill);
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.55f));
		Blur->SetContent(Dim);

		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
		Width->SetWidthOverride(MvsMetrics::ModalWidth);
		UOverlaySlot* WidthSlot = Root->AddChildToOverlay(Width);
		WidthSlot->SetHorizontalAlignment(HAlign_Center);
		WidthSlot->SetVerticalAlignment(VAlign_Center);

		Panel = WidgetTree->ConstructWidget<UMvsPanel>();
		Panel->SetPanelPadding(MvsMetrics::ModalPadding);
		Panel->SetShape(16.f, EMvsChamfer::Opposite);
		Width->SetContent(Panel);
		SlideTarget = Panel;
		SlideFrom = FVector2D(0.f, 18.f);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		CautionText = MakeText(LOCTEXT("Caution", "Caution"), EMvsTextStyle::Label, EMvsColorToken::Danger);
		CautionText->SetVisibility(ESlateVisibility::Collapsed);
		Column->AddChildToVerticalBox(CautionText);

		TitleText = MakeText(FText::GetEmpty(), EMvsTextStyle::Title, EMvsColorToken::TextPrimary);
		TitleText->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(TitleText)->SetPadding(MvsMetrics::TitlePadding);

		BodyText = MakeText(FText::GetEmpty(), EMvsTextStyle::Body, EMvsColorToken::TextMuted);
		BodyText->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(BodyText)->SetPadding(FMargin(0.f, 0.f, 0.f, 26.f));

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons)->SetHorizontalAlignment(HAlign_Right);

		YesButton = WidgetTree->ConstructWidget<UMvsButton>();
		YesButton->SetLabel(LOCTEXT("Yes", "Yes"));
		YesButton->OnClicked().AddLambda([this]() { Finish(true); });
		Buttons->AddChildToHorizontalBox(YesButton)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

		UMvsButton* No = WidgetTree->ConstructWidget<UMvsButton>();
		No->SetLabel(LOCTEXT("No", "No"));
		No->OnClicked().AddLambda([this]() { Finish(false); });
		Buttons->AddChildToHorizontalBox(No);

		// Safe default: focus lands on "No".
		DefaultFocus = No;
	}
	return Super::RebuildWidget();
}

void UConfirmModalScreen::Setup(const FText& InTitle, const FText& InBody, FOnConfirmResult InCallback, bool bInDestructive)
{
	// The layer stack pools its screens, so a confirmation opened before may come back here already answered.
	Callback = MoveTemp(InCallback);
	bAnswered = false;
	bDestructive = bInDestructive;
	if (TitleText)
	{
		TitleText->SetText(InTitle);
		BodyText->SetText(InBody);
		CautionText->SetVisibility(bDestructive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		YesButton->SetKind(bDestructive ? EMvsButtonKind::Danger : EMvsButtonKind::Standard);
	}
	ApplyTheme(MvsStyle::Theme(this));
}

void UConfirmModalScreen::ApplyTheme(const FMvsTheme& Theme)
{
	Super::ApplyTheme(Theme);
	if (Panel)
	{
		const EMvsColorToken Accent = bDestructive ? EMvsColorToken::Danger : EMvsColorToken::Accent;
		Panel->SetColors(Theme.Color(EMvsColorToken::Panel, 0.96f), Theme.Color(Accent, 0.7f));
		Panel->SetAccent(Theme.Color(Accent), 4.f);
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

void UConfirmModalScreen::NativeOnClosed()
{
	// Back / Esc counts as "No" so callers always get exactly one answer.
	if (!bAnswered)
	{
		bAnswered = true;
		Callback.ExecuteIfBound(false);
	}
	Super::NativeOnClosed();
}

#undef LOCTEXT_NAMESPACE
