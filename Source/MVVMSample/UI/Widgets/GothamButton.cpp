// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamButton.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"
#include "UI/GothamWidgetTick.h"

UGothamButtonStyle::UGothamButtonStyle()
{
	auto Box = [](const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = Color;
		Brush.Margin = FMargin(0.f);
		return Brush;
	};

	NormalBase = Box(FLinearColor(0.03f, 0.03f, 0.04f, 0.85f));
	NormalHovered = Box(FLinearColor(0.85f, 0.7f, 0.15f, 0.9f));
	NormalPressed = Box(FLinearColor(0.6f, 0.5f, 0.1f, 0.95f));
	SelectedBase = NormalHovered;
	SelectedHovered = NormalHovered;
	SelectedPressed = NormalPressed;
	Disabled = Box(FLinearColor(0.02f, 0.02f, 0.02f, 0.4f));
	ButtonPadding = FMargin(24.f, 10.f);
}

UGothamButton::UGothamButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Style = UGothamButtonStyle::StaticClass();
	SetIsFocusable(true);
}

bool UGothamButton::Initialize()
{
	// UCommonButtonBase only wires its internal button (click, focus, style) if the widget tree already has a
	// root when it initializes. There is no designer asset here, so build the content first.
	if (!WidgetTree && !HasAnyFlags(RF_ClassDefaultObject))
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);

		UBorder* Content = WidgetTree->ConstructWidget<UBorder>();
		Content->SetBrushColor(FLinearColor::Transparent);
		Content->SetPadding(FMargin(24.f, 10.f));
		Content->SetHorizontalAlignment(HAlign_Center);
		Content->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Content;

		Label = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 20;
		Label->SetFont(Font);
		Label->SetJustification(ETextJustify::Center);
		Label->SetText(PendingLabel);
		Content->SetContent(Label);
	}
	return Super::Initialize();
}

void UGothamButton::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
}

void UGothamButton::SetLabel(const FText& InLabel)
{
	PendingLabel = InLabel;
	if (Label)
	{
		Label->SetText(InLabel);
	}
}
