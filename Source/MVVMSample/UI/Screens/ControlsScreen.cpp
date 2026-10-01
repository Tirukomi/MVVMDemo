// Copyright IG. All Rights Reserved.

#include "UI/Screens/ControlsScreen.h"

#include "UI/Style/MvsMetrics.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "Input/MvsBindingStore.h"
#include "Input/MvsUIInput.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "UI/Widgets/MvsMenuList.h"
#include "ViewModels/ControlsViewModel.h"
#include "ViewModels/MvsMVVM.h"

#define LOCTEXT_NAMESPACE "Mvs.ControlsScreen"

TSharedRef<SWidget> UControlsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Options"), LOCTEXT("Title", "Controls"));

		Status = MakeText(FText::GetEmpty(), EMvsTextStyle::Label, EMvsColorToken::Warning);
		Status->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(MvsMetrics::ItemIndent, 0.f, 0.f, 10.f));

		constexpr float LabelWidth = 300.f;
		constexpr float SlotWidth = 230.f;

		// Header: which column is which device.
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Header)->SetPadding(FMargin(MvsMetrics::ItemIndent, 0.f, 0.f, 6.f));
		auto AddHeaderCell = [&](const FText& Text, float Width)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
			Box->SetWidthOverride(Width);
			UTextBlock* Cell = MakeText(Text, EMvsTextStyle::Label, EMvsColorToken::TextMuted);
			Cell->SetJustification(ETextJustify::Center);
			Box->SetContent(Cell);
			Header->AddChildToHorizontalBox(Box);
		};
		AddHeaderCell(FText::GetEmpty(), LabelWidth);
		AddHeaderCell(LOCTEXT("KeyboardColumn", "Keyboard / Mouse"), SlotWidth);
		AddHeaderCell(LOCTEXT("GamepadColumn", "Gamepad"), SlotWidth);

		// The list takes the frame's remaining height and scrolls inside it.
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		// Keep the focused row on screen as gamepad / keyboard focus moves.
		Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
		UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
		ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ScrollSlot->SetHorizontalAlignment(HAlign_Left);
		// Each binding row is one highlight item; the focused slot inside it also glows.
		UMvsMenuList* List = WidgetTree->ConstructWidget<UMvsMenuList>();
		Scroll->AddChild(List);

		SlotButtons.Reset();
		for (const FMvsBindingDef& Def : MvsBindings::GetDefinitions())
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			List->AddItem(Row);

			USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>();
			LabelBox->SetWidthOverride(LabelWidth);
			UTextBlock* Label = MakeText(Def.DisplayName, EMvsTextStyle::BodyStrong, EMvsColorToken::TextPrimary);
			Label->SetAutoWrapText(true);
			LabelBox->SetContent(Label);
			UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(MvsMetrics::ItemIndent, 0.f, 0.f, 0.f));

			for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
			{
				USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>();
				Cell->SetWidthOverride(SlotWidth);
				Row->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(4.f, 3.f));
				if (SlotIndex == MvsBindings::GamepadSlot && !Def.bHasGamepadSlot)
				{
					continue; // e.g. WASD directions: the left stick is fixed, so there is nothing to rebind
				}
				UMvsButton* Button = WidgetTree->ConstructWidget<UMvsButton>();
				const FName Name = Def.Name;
				Button->OnClicked().AddLambda([this, Name, SlotIndex]() { BeginCapture(Name, SlotIndex); });
				Cell->SetContent(Button);
				SlotButtons.Add({ Name, SlotIndex, Button });
				if (!DefaultFocus)
				{
					DefaultFocus = Button;
				}
			}
		}

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
		UMvsButton* Reset = WidgetTree->ConstructWidget<UMvsButton>();
		Reset->SetLabel(LOCTEXT("ResetAll", "Reset to defaults"));
		Reset->OnClicked().AddLambda([this]() { ResetAll(); });
		Buttons->AddChildToHorizontalBox(Reset)->SetPadding(FMargin(0.f, 0.f, MvsMetrics::ButtonGap, 0.f));
		UMvsButton* Back = WidgetTree->ConstructWidget<UMvsButton>();
		Back->SetLabel(LOCTEXT("Back", "Back"));
		Back->OnClicked().AddLambda([this]() { DeactivateWidget(); });
		Buttons->AddChildToHorizontalBox(Back);

		AddFooter(MakeActionBar(LOCTEXT("Rebind", "Rebind"), LOCTEXT("BackHint", "Back")));
	}
	return Super::RebuildWidget();
}

void UControlsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	ViewModel = NewObject<UControlsViewModel>(this);
	using FVM = UControlsViewModel::FFieldNotificationClassDescriptor;
	MvsMVVM::Bind(ViewModel, this, &UControlsScreen::OnViewModelChanged, { FVM::Revision, FVM::StatusText });
	ViewModel->SetStore(MvsBindings::MakeEnhancedInputStore(GetOwningLocalPlayer()));
}

void UControlsScreen::NativeDestruct()
{
	MvsMVVM::Unbind(ViewModel, this);
	Super::NativeDestruct();
}

void UControlsScreen::ResetAll()
{
	EndCapture();
	// Resetting throws away every rebind at once, so it asks first.
	auto* UI = GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>();
	TSubclassOf<UConfirmModalScreen> ModalClass = UConfirmModalScreen::StaticClass();
	if (UConfirmModalScreen* Modal = UI ? UI->PushScreen<UConfirmModalScreen>(EMvsUILayer::Modal, ModalClass) : nullptr)
	{
		Modal->Setup(LOCTEXT("ResetTitle", "Reset all controls?"), LOCTEXT("ResetBody", "Every key and button goes back to its default."),
			FOnConfirmResult::CreateUObject(this, &UControlsScreen::OnResetConfirmed), /*bDestructive*/ true);
	}
}

void UControlsScreen::OnResetConfirmed(bool bConfirmed)
{
	if (bConfirmed)
	{
		ViewModel->ResetToDefaults();
	}
}

void UControlsScreen::BeginCapture(FName Name, int32 SlotIndex)
{
	bCapturing = true;
	CaptureName = Name;
	CaptureSlot = SlotIndex;
	ViewModel->SetStatus(SlotIndex == MvsBindings::GamepadSlot
		? LOCTEXT("PressPad", "Press a gamepad button. Choose B to cancel.")
		: LOCTEXT("PressKey", "Press a key or mouse button. Choose Esc to cancel."));
	RefreshLabels();
}

void UControlsScreen::EndCapture()
{
	bCapturing = false;
	RefreshLabels();
}

bool UControlsScreen::HandleCapturedKey(const FKey& Key)
{
	// The back keys cancel (they are reserved and cannot be bound, see MvsBindings::IsKeyAllowedForSlot).
	const auto* Enhanced = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (Enhanced && Enhanced->QueryKeysMappedToAction(UMvsUIInputData::Get().GetBackAction()).Contains(Key))
	{
		ViewModel->SetStatus(FText::GetEmpty());
		EndCapture();
		return true;
	}
	if (ViewModel->RequestRebind(CaptureName, CaptureSlot, Key))
	{
		EndCapture();
	}
	return true; // an invalid key keeps capturing so the player can try another, with the reason shown
}

FReply UControlsScreen::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bCapturing)
	{
		HandleCapturedKey(InKeyEvent.GetKey());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UControlsScreen::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bCapturing && CaptureSlot == MvsBindings::KeyboardSlot)
	{
		HandleCapturedKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UControlsScreen::RefreshLabels()
{
	if (!ViewModel || !Status)
	{
		return;
	}
	Status->SetText(ViewModel->GetStatusText());
	for (const FSlotButton& Entry : SlotButtons)
	{
		if (!Entry.Button)
		{
			continue;
		}
		const bool bThis = bCapturing && Entry.Name == CaptureName && Entry.Slot == CaptureSlot;
		const FKey Key = ViewModel->GetKey(Entry.Name, Entry.Slot);
		Entry.Button->SetLabel(bThis ? LOCTEXT("Capturing", "...") : (Key.IsValid() ? UMvsInputGlyph::GetKeyLabel(Key, GetOwningLocalPlayer()) : LOCTEXT("Unbound", "Unbound")));
	}
}

#undef LOCTEXT_NAMESPACE
