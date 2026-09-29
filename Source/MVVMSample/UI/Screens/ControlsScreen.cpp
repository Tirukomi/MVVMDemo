// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/ControlsScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ControlsViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamControls, Log, All);

#define LOCTEXT_NAMESPACE "Gotham.ControlsScreen"

TSharedRef<SWidget> UControlsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
		Dim->SetContent(Column);
		Column->AddChildToVerticalBox(MakeTitle(LOCTEXT("Title", "CONTROLS")));

		Status = WidgetTree->ConstructWidget<UTextBlock>();
		Status->SetJustification(ETextJustify::Center);
		Status->SetAutoWrapText(true);
		Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0.f, 4.f, 0.f, 12.f));

		USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetMaxDesiredHeight(360.f);
		Column->AddChildToVerticalBox(Height);
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
		Height->SetContent(Scroll);

		// Header: which column is which device.
		UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
		Scroll->AddChild(Header);
		auto AddHeaderCell = [&](const FText& Text, float Width)
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
			Box->SetWidthOverride(Width);
			UTextBlock* Cell = WidgetTree->ConstructWidget<UTextBlock>();
			Cell->SetText(Text);
			Cell->SetJustification(ETextJustify::Center);
			Box->SetContent(Cell);
			Header->AddChildToHorizontalBox(Box);
		};
		AddHeaderCell(FText::GetEmpty(), 260.f);
		AddHeaderCell(LOCTEXT("KeyboardColumn", "Keyboard / Mouse"), 220.f);
		AddHeaderCell(LOCTEXT("GamepadColumn", "Gamepad"), 220.f);

		SlotButtons.Reset();
		for (const FGothamBindingDef& Def : GothamBindings::GetDefinitions())
		{
			UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
			Scroll->AddChild(Row);

			USizeBox* LabelBox = WidgetTree->ConstructWidget<USizeBox>();
			LabelBox->SetWidthOverride(260.f);
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
			Label->SetText(Def.DisplayName);
			Label->SetAutoWrapText(true);
			LabelBox->SetContent(Label);
			Row->AddChildToHorizontalBox(LabelBox)->SetVerticalAlignment(VAlign_Center);

			for (int32 SlotIndex = 0; SlotIndex < 2; ++SlotIndex)
			{
				USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>();
				Cell->SetWidthOverride(220.f);
				Row->AddChildToHorizontalBox(Cell)->SetPadding(FMargin(2.f));
				if (SlotIndex == GothamBindings::GamepadSlot && !Def.bHasGamepadSlot)
				{
					continue; // e.g. WASD directions: the left stick is fixed, so there is nothing to rebind
				}
				UGothamButton* Button = WidgetTree->ConstructWidget<UGothamButton>();
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
		Column->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		UGothamButton* Reset = WidgetTree->ConstructWidget<UGothamButton>();
		Reset->SetLabel(LOCTEXT("ResetAll", "Reset to defaults"));
		Reset->OnClicked().AddLambda([this]() { ResetAll(); });
		Buttons->AddChildToHorizontalBox(Reset)->SetPadding(FMargin(4.f, 0.f));
		UGothamButton* Back = WidgetTree->ConstructWidget<UGothamButton>();
		Back->SetLabel(LOCTEXT("Back", "Back"));
		Back->OnClicked().AddLambda([this]() { DeactivateWidget(); });
		Buttons->AddChildToHorizontalBox(Back)->SetPadding(FMargin(4.f, 0.f));

		Column->AddChildToVerticalBox(MakeHintBar(LOCTEXT("Rebind", "Rebind"), LOCTEXT("BackHint", "Back")))
			->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	}
	return Super::RebuildWidget();
}

UEnhancedInputUserSettings* UControlsScreen::GetUserSettings() const
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	return Input ? Input->GetUserSettings() : nullptr;
}

void UControlsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	ViewModel = NewObject<UControlsViewModel>(this);
	using FVM = UControlsViewModel::FFieldNotificationClassDescriptor;
	const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UControlsScreen::OnViewModelChanged);
	ViewModel->AddFieldValueChangedDelegate(FVM::Revision, Delegate);
	ViewModel->AddFieldValueChangedDelegate(FVM::StatusText, Delegate);
	ViewModel->OnChangesPlanned.AddUObject(this, &UControlsScreen::ApplyChanges);

	PullSnapshot();
}

void UControlsScreen::NativeDestruct()
{
	if (ViewModel)
	{
		ViewModel->RemoveAllFieldValueChangedDelegates(this);
		ViewModel->OnChangesPlanned.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UControlsScreen::PullSnapshot()
{
	TArray<FGothamBindingSlot> Snapshot;
	UEnhancedInputUserSettings* UserSettings = GetUserSettings();
	UE_LOG(LogGothamControls, Log, TEXT("User settings: %s"), UserSettings ? TEXT("available") : TEXT("MISSING (is bEnableUserSettings on?)"));
	if (UserSettings)
	{
		UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile();
		UE_LOG(LogGothamControls, Log, TEXT("Active profile: %s, rows: %d"), Profile ? *Profile->GetProfileIdString() : TEXT("none"), Profile ? Profile->GetPlayerMappingRows().Num() : 0);
		if (Profile)
		{
			for (const TPair<FName, FKeyMappingRow>& Pair : Profile->GetPlayerMappingRows())
			{
				for (const FPlayerKeyMapping& Mapping : Pair.Value.Mappings)
				{
					Snapshot.Add({ Mapping.GetMappingName(), static_cast<int32>(Mapping.GetSlot()), Mapping.GetCurrentKey() });
				}
			}
		}
	}
	ViewModel->SetSnapshot(Snapshot);
}

void UControlsScreen::ApplyChanges(const TArray<FGothamBindingChange>& Changes)
{
	if (UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		for (const FGothamBindingChange& Change : Changes)
		{
			FMapPlayerKeyArgs Args;
			Args.MappingName = Change.Name;
			Args.Slot = static_cast<EPlayerMappableKeySlot>(Change.Slot);
			Args.NewKey = Change.NewKey;
			FGameplayTagContainer Failure;
			UserSettings->MapPlayerKey(Args, Failure);
		}
		UserSettings->ApplySettings();
		UserSettings->SaveSettings();
	}
	PullSnapshot();
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->NotifyBindingsChanged();
	}
}

void UControlsScreen::ResetAll()
{
	if (UEnhancedInputUserSettings* UserSettings = GetUserSettings())
	{
		if (UEnhancedPlayerMappableKeyProfile* Profile = UserSettings->GetActiveKeyProfile())
		{
			Profile->ResetToDefault();
		}
		UserSettings->ApplySettings();
		UserSettings->SaveSettings();
	}
	EndCapture();
	ViewModel->SetStatus(LOCTEXT("WasReset", "Controls reset to defaults."));
	PullSnapshot();
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->NotifyBindingsChanged();
	}
}

void UControlsScreen::BeginCapture(FName Name, int32 SlotIndex)
{
	bCapturing = true;
	CaptureName = Name;
	CaptureSlot = SlotIndex;
	ViewModel->SetStatus(SlotIndex == GothamBindings::GamepadSlot
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
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
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
	if (bCapturing && CaptureSlot == GothamBindings::KeyboardSlot)
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
		Entry.Button->SetLabel(bThis ? LOCTEXT("Capturing", "...") : (Key.IsValid() ? UGothamInputGlyph::GetKeyLabel(Key) : LOCTEXT("Unbound", "Unbound")));
	}
}

#undef LOCTEXT_NAMESPACE
