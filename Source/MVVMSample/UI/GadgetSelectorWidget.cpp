// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GadgetSelectorWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/GothamHudPrimitives.h"
#include "ViewModels/GadgetViewModels.h"

#define LOCTEXT_NAMESPACE "Gotham.GadgetSelector"

TSharedRef<SWidget> UGadgetSelectorWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		WidgetTree->RootWidget = Column;

		// Selected gadget: text block to the left of the big icon, right-aligned against the screen edge.
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Top)->SetHorizontalAlignment(HAlign_Right);

		UVerticalBox* Text = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* TextSlot = Top->AddChildToHorizontalBox(Text);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

		SelectedName = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedName->SetJustification(ETextJustify::Right);
		Text->AddChildToVerticalBox(SelectedName)->SetHorizontalAlignment(HAlign_Right);

		SelectedState = WidgetTree->ConstructWidget<UTextBlock>();
		SelectedState->SetJustification(ETextJustify::Right);
		Text->AddChildToVerticalBox(SelectedState)->SetHorizontalAlignment(HAlign_Right);

		SelectedIcon = WidgetTree->ConstructWidget<UGadgetIcon>();
		SelectedIcon->IconSize = 76.f;
		Top->AddChildToHorizontalBox(SelectedIcon);

		OthersRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(OthersRow)->SetHorizontalAlignment(HAlign_Right);
	}
	return Super::RebuildWidget();
}

void UGadgetSelectorWidget::SetViewModel(UGadgetBarViewModel* InViewModel)
{
	UnbindSlots();
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UGadgetBarViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGadgetSelectorWidget::OnBarChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::Slots, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::SelectedIndex, Delegate);
	}
	RebuildEntries();
}

void UGadgetSelectorWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UGadgetSelectorWidget::OnBarChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	if (FieldId == UGadgetBarViewModel::FFieldNotificationClassDescriptor::Slots)
	{
		RebuildEntries();
	}
	else
	{
		Refresh();
	}
}

void UGadgetSelectorWidget::OnSlotChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	// Cooldowns tick every frame while recharging; readiness, names and selection are rare and restyle everything.
	if (FieldId == UGadgetSlotViewModel::FFieldNotificationClassDescriptor::CooldownPercent)
	{
		RefreshCooldowns();
		return;
	}
	Refresh();
}

void UGadgetSelectorWidget::RefreshCooldowns()
{
	if (!ViewModel || !SelectedIcon)
	{
		return;
	}
	if (const UGadgetSlotViewModel* Current = ViewModel->GetSlot(ViewModel->GetSelectedIndex()))
	{
		SelectedIcon->SetCooldown(Current->GetCooldownPercent());
		const int32 Seconds = FMath::CeilToInt(Current->GetCooldownRemaining());
		if (!Current->GetIsReady() && Seconds != LastSeconds)
		{
			SelectedState->SetText(FText::Format(LOCTEXT("Recharging", "{0}s"), FText::AsNumber(Seconds)));
		}
		LastSeconds = Current->GetIsReady() ? -1 : Seconds;
	}
	for (int32 i = 0; i < SmallIcons.Num(); ++i)
	{
		if (const UGadgetSlotViewModel* SlotVM = ViewModel->GetSlot(i))
		{
			SmallIcons[i]->SetCooldown(SlotVM->GetCooldownPercent());
		}
	}
}

void UGadgetSelectorWidget::UnbindSlots()
{
	if (ViewModel)
	{
		for (UGadgetSlotViewModel* SlotVM : ViewModel->GetSlots())
		{
			SlotVM->RemoveAllFieldValueChangedDelegates(this);
		}
	}
}

void UGadgetSelectorWidget::RebuildEntries()
{
	if (!OthersRow)
	{
		return;
	}
	UnbindSlots();
	OthersRow->ClearChildren();
	SmallIcons.Reset();
	SmallKeys.Reset();
	if (!ViewModel)
	{
		return;
	}

	using FSlotVM = UGadgetSlotViewModel::FFieldNotificationClassDescriptor;
	const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGadgetSelectorWidget::OnSlotChanged);
	for (UGadgetSlotViewModel* SlotVM : ViewModel->GetSlots())
	{
		SlotVM->AddFieldValueChangedDelegate(FSlotVM::CooldownPercent, Delegate);
		SlotVM->AddFieldValueChangedDelegate(FSlotVM::bIsReady, Delegate);
		SlotVM->AddFieldValueChangedDelegate(FSlotVM::DisplayName, Delegate);

		// Small entry: key above the icon.
		UVerticalBox* Entry = WidgetTree->ConstructWidget<UVerticalBox>();
		OthersRow->AddChildToHorizontalBox(Entry)->SetPadding(FMargin(8.f, 6.f, 0.f, 0.f));
		UTextBlock* Key = WidgetTree->ConstructWidget<UTextBlock>();
		Key->SetJustification(ETextJustify::Center);
		Entry->AddChildToVerticalBox(Key)->SetHorizontalAlignment(HAlign_Center);
		UGadgetIcon* Icon = WidgetTree->ConstructWidget<UGadgetIcon>();
		Icon->IconSize = 34.f;
		Entry->AddChildToVerticalBox(Icon)->SetHorizontalAlignment(HAlign_Center);
		SmallIcons.Add(Icon);
		SmallKeys.Add(Key);
	}
	Refresh();
}

void UGadgetSelectorWidget::Refresh()
{
	if (!ViewModel || !SelectedIcon)
	{
		return;
	}
	const int32 Selected = ViewModel->GetSelectedIndex();
	const UGadgetSlotViewModel* Current = ViewModel->GetSlot(Selected);
	SetVisibility(Current ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!Current)
	{
		return;
	}

	const FLinearColor Primary = GetToken(EGothamColorToken::TextPrimary);
	const FLinearColor Muted = GetToken(EGothamColorToken::TextMuted);
	FLinearColor Ring = GetToken(EGothamColorToken::PanelEdge);
	Ring.A = 0.5f;

	SelectedIcon->SetIconIndex(Current->GetIconIndex());
	SelectedIcon->SetColors(Current->GetIsReady() ? GetToken(EGothamColorToken::Accent) : Primary, Ring);
	SelectedIcon->SetCooldown(Current->GetCooldownPercent());

	GothamStyle::ApplyText(SelectedName, EGothamTextStyle::Header, Primary);
	SelectedName->SetText(Current->GetDisplayName());
	GothamStyle::ApplyText(SelectedState, EGothamTextStyle::Label, Current->GetIsReady() ? GetToken(EGothamColorToken::Accent) : Muted);
	SelectedState->SetText(Current->GetIsReady()
		? LOCTEXT("Ready", "Ready")
		: FText::Format(LOCTEXT("Recharging", "{0}s"), FText::AsNumber(FMath::CeilToInt(Current->GetCooldownRemaining()))));

	for (int32 i = 0; i < SmallIcons.Num(); ++i)
	{
		const UGadgetSlotViewModel* SlotVM = ViewModel->GetSlot(i);
		if (!SlotVM)
		{
			continue;
		}
		const bool bIsSelected = i == Selected;
		SmallIcons[i]->SetIconIndex(SlotVM->GetIconIndex());
		SmallIcons[i]->SetColors(bIsSelected ? GetToken(EGothamColorToken::Accent) : (SlotVM->GetIsReady() ? Primary : Muted), Ring);
		SmallIcons[i]->SetCooldown(SlotVM->GetCooldownPercent());
		GothamStyle::ApplyText(SmallKeys[i], EGothamTextStyle::Key, bIsSelected ? GetToken(EGothamColorToken::Accent) : Muted);
		SmallKeys[i]->SetText(SlotVM->GetHotkey());
	}

	if (Selected != LastSelected && LastSelected != INDEX_NONE)
	{
		GothamMotion::Pop(SelectedIcon, 1.15f);
	}
	LastSelected = Selected;
}

#undef LOCTEXT_NAMESPACE
