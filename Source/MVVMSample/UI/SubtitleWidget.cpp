// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SubtitleWidget.h"
#include "UI/Style/GothamStyle.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamPanel.h"
#include "UI/Widgets/GothamText.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/SubtitleViewModel.h"

TSharedRef<SWidget> USubtitleWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetMaxDesiredWidth(900.f);
		WidgetTree->RootWidget = Size;

		Panel = WidgetTree->ConstructWidget<UGothamPanel>();
		Panel->SetPanelPadding(FMargin(20.f, 10.f));
		Panel->SetShape(8.f, EGothamChamfer::Opposite);
		Size->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		SpeakerText = WidgetTree->ConstructWidget<UGothamText>();
		Column->AddChildToVerticalBox(SpeakerText);

		LineText = WidgetTree->ConstructWidget<UGothamText>();
		LineText->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(LineText);
	}
	return Super::RebuildWidget();
}

void USubtitleWidget::SetViewModel(USubtitleViewModel* InViewModel)
{
	using FVM = USubtitleViewModel::FFieldNotificationClassDescriptor;
	GothamMVVM::Unbind(ViewModel, this);
	ViewModel = InViewModel;
	GothamMVVM::Bind(ViewModel, this, &USubtitleWidget::OnFieldChanged,
		{ FVM::Line, FVM::Speaker, FVM::bIsVisible, FVM::FontSize, FVM::bHasBackground });
	Refresh();
}

void USubtitleWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void USubtitleWidget::Refresh()
{
	if (!ViewModel || !Panel)
	{
		return;
	}
	SetVisibility(ViewModel->GetIsVisible() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	GothamStyle::ApplyText(SpeakerText, EGothamTextStyle::Label, GetToken(EGothamColorToken::Accent));
	FSlateFontInfo SpeakerFont = SpeakerText->GetFont();
	SpeakerFont.Size = FMath::Max(12, ViewModel->GetFontSize() - 6);
	SpeakerText->SetFont(SpeakerFont);

	SpeakerText->SetText(ViewModel->GetSpeaker());
	LineText->SetText(ViewModel->GetLine());

	// Backing panel is optional; in high contrast it is near-opaque so the text always has a solid backdrop.
	FLinearColor Fill = GetToken(EGothamColorToken::Panel);
	Fill.A = GetPanelAlpha() * 0.85f;
	FLinearColor Edge = GetToken(EGothamColorToken::PanelEdge);
	Edge.A = 0.5f;
	Panel->SetColors(ViewModel->GetHasBackground() ? Fill : FLinearColor::Transparent, ViewModel->GetHasBackground() ? Edge : FLinearColor::Transparent);
	GothamStyle::ApplyText(LineText, EGothamTextStyle::Body, GetToken(EGothamColorToken::TextPrimary));
	FSlateFontInfo SizedFont = LineText->GetFont();
	SizedFont.Size = ViewModel->GetFontSize();
	LineText->SetFont(SizedFont);
}
