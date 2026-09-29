// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SubtitleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "ViewModels/SubtitleViewModel.h"

TSharedRef<SWidget> USubtitleWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetMaxDesiredWidth(900.f);
		WidgetTree->RootWidget = Size;

		Panel = WidgetTree->ConstructWidget<UBorder>();
		Panel->SetPadding(FMargin(20.f, 10.f));
		Size->SetContent(Panel);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Panel->SetContent(Column);

		SpeakerText = WidgetTree->ConstructWidget<UTextBlock>();
		SpeakerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.4f, 0.85f, 1.f)));
		Column->AddChildToVerticalBox(SpeakerText);

		LineText = WidgetTree->ConstructWidget<UTextBlock>();
		LineText->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(LineText);
	}
	return Super::RebuildWidget();
}

void USubtitleWidget::SetViewModel(USubtitleViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = USubtitleViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &USubtitleWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::Line, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::Speaker, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bIsVisible, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::FontSize, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::bHasBackground, Delegate);
	}
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

	FSlateFontInfo LineFont = LineText->GetFont();
	LineFont.Size = ViewModel->GetFontSize();
	LineText->SetFont(LineFont);
	FSlateFontInfo SpeakerFont = SpeakerText->GetFont();
	SpeakerFont.Size = FMath::Max(12, ViewModel->GetFontSize() - 6);
	SpeakerText->SetFont(SpeakerFont);

	SpeakerText->SetText(ViewModel->GetSpeaker());
	LineText->SetText(ViewModel->GetLine());
	SpeakerText->SetColorAndOpacity(FSlateColor(GetToken(EGothamColorToken::Info)));

	// Backing panel is optional; in high contrast it is near-opaque so the text always has a solid backdrop.
	Panel->SetBrushColor(ViewModel->GetHasBackground() ? FLinearColor(0.f, 0.f, 0.f, GetPanelAlpha() * 0.85f) : FLinearColor::Transparent);
}
