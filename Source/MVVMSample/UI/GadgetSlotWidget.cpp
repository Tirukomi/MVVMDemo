// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GadgetSlotWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/GothamWidgetTick.h"
#include "ViewModels/GadgetViewModels.h"

#define LOCTEXT_NAMESPACE "Gotham.GadgetSlot"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, UOverlay* Parent, int32 Size, EHorizontalAlignment H, EVerticalAlignment V)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		UOverlaySlot* Slot = Parent->AddChildToOverlay(Text);
		Slot->SetHorizontalAlignment(H);
		Slot->SetVerticalAlignment(V);
		Slot->SetPadding(FMargin(6.f));
		return Text;
	}
}

TSharedRef<SWidget> UGadgetSlotWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(112.f);
		Size->SetHeightOverride(96.f);
		WidgetTree->RootWidget = Size;

		Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.75f));
		Size->SetContent(Frame);

		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
		Frame->SetContent(Overlay);

		// Cooldown fills from the bottom, draining as the gadget recharges.
		Cooldown = WidgetTree->ConstructWidget<UProgressBar>();
		Cooldown->SetBarFillType(EProgressBarFillType::BottomToTop);
		Cooldown->SetFillColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.65f));
		UOverlaySlot* BarSlot = Overlay->AddChildToOverlay(Cooldown);
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);

		KeyText = MakeText(WidgetTree, Overlay, 14, HAlign_Left, VAlign_Top);
		NameText = MakeText(WidgetTree, Overlay, 11, HAlign_Center, VAlign_Bottom);
		TimeText = MakeText(WidgetTree, Overlay, 22, HAlign_Center, VAlign_Center);
	}
	return Super::RebuildWidget();
}

void UGadgetSlotWidget::SetViewModel(UGadgetSlotViewModel* InViewModel)
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
	}
	ViewModel = InViewModel;
	if (ViewModel)
	{
		using FVM = UGadgetSlotViewModel::FFieldNotificationClassDescriptor;
		const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGadgetSlotWidget::OnFieldChanged);
		ViewModel->AddFieldValueChangedDelegate(FVM::CooldownPercent, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::CooldownRemaining, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::DisplayName, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::Hotkey, Delegate);
		ViewModel->AddFieldValueChangedDelegate(FVM::Tint, Delegate);
	}
	Refresh();
}

void UGadgetSlotWidget::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
}

void UGadgetSlotWidget::NativeDestruct()
{
	SetViewModel(nullptr);
	Super::NativeDestruct();
}

void UGadgetSlotWidget::Refresh()
{
	if (!Frame || !ViewModel)
	{
		return;
	}
	const FLinearColor Tint = ViewModel->GetTint();
	Frame->SetBrushColor(ViewModel->GetIsReady() ? Tint * 0.35f + FLinearColor(0.f, 0.f, 0.f, 0.6f) : FLinearColor(0.02f, 0.02f, 0.03f, 0.75f));
	Cooldown->SetPercent(ViewModel->GetCooldownPercent());
	NameText->SetText(ViewModel->GetDisplayName());
	KeyText->SetText(ViewModel->GetHotkey());
	TimeText->SetText(ViewModel->GetIsReady()
		? FText::GetEmpty()
		: FText::AsNumber(FMath::CeilToInt(ViewModel->GetCooldownRemaining())));
}

#undef LOCTEXT_NAMESPACE
