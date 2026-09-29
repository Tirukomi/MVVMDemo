// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/GadgetWheelScreen.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Core/GothamCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Style/GothamStyle.h"
#include "UI/Widgets/GadgetWheel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"

namespace
{
	/** World time scale while the wheel is open. UI animation runs on real time and is unaffected. */
	constexpr float WheelTimeDilation = 0.1f;
}

UGadgetWheelScreen::UGadgetWheelScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UGadgetWheelScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Dim = WidgetTree->ConstructWidget<UBorder>();
		Dim->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
		Dim->SetHorizontalAlignment(HAlign_Center);
		Dim->SetVerticalAlignment(VAlign_Center);
		WidgetTree->RootWidget = Dim;

		Wheel = WidgetTree->ConstructWidget<UGadgetWheel>();
		Wheel->WheelStyle.LabelFont = GothamStyle::Font(EGothamTextStyle::Header);
		Wheel->OnItemSelected.AddDynamic(this, &UGadgetWheelScreen::HandleItemSelected);
		Dim->SetContent(Wheel);
		DefaultFocus = Wheel;
	}
	return Super::RebuildWidget();
}

void UGadgetWheelScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>())
		{
			GadgetBar = ViewModels->GetGadgetBar();
			using FSlotVM = UGadgetSlotViewModel::FFieldNotificationClassDescriptor;
			const auto Delegate = INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UGadgetWheelScreen::OnSlotChanged);
			for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
			{
				SlotVM->AddFieldValueChangedDelegate(FSlotVM::CooldownPercent, Delegate);
				SlotVM->AddFieldValueChangedDelegate(FSlotVM::bIsReady, Delegate);
			}
		}
	}
	if (const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		Wheel->bReduceMotion = Settings->GetSettings().bReducedMotion;
		Wheel->SynchronizeProperties();
	}
	RefreshItems();
}

void UGadgetWheelScreen::NativeDestruct()
{
	if (GadgetBar)
	{
		for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
		{
			SlotVM->RemoveAllFieldValueChangedDelegates(this);
		}
	}
	Super::NativeDestruct();
}

void UGadgetWheelScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	UGameplayStatics::SetGlobalTimeDilation(this, WheelTimeDilation);
}

void UGadgetWheelScreen::NativeOnDeactivated()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
	Super::NativeOnDeactivated();
}

void UGadgetWheelScreen::SetStickInput(FVector2D Stick)
{
	if (Wheel)
	{
		Wheel->SetStickInput(Stick);
	}
}

void UGadgetWheelScreen::RefreshItems()
{
	if (!GadgetBar || !Wheel)
	{
		return;
	}
	TArray<FGothamWheelItem> Items;
	for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
	{
		FGothamWheelItem& Item = Items.AddDefaulted_GetRef();
		Item.Label = SlotVM->GetDisplayName();
		Item.Tint = SlotVM->GetTint();
		Item.bReady = SlotVM->GetIsReady();
		Item.CooldownPercent = SlotVM->GetCooldownPercent();
	}
	Wheel->SetItems(Items);
}

namespace
{
	bool IsOpenKey(const FKey& Key)
	{
		return Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder;
	}
}

FReply UGadgetWheelScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Toggle mode: pressing the open key again commits (or closes if nothing is hovered).
	const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this);
	if (Settings && Settings->GetSettings().WheelMode == EGothamWheelMode::Toggle && IsOpenKey(InKeyEvent.GetKey()) && !InKeyEvent.IsRepeat())
	{
		if (!Wheel->CommitHovered())
		{
			DeactivateWidget();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UGadgetWheelScreen::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Hold mode: releasing the key that opened the wheel commits the hovered gadget.
	const FKey Key = InKeyEvent.GetKey();
	const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this);
	const bool bHoldMode = !Settings || Settings->GetSettings().WheelMode == EGothamWheelMode::Hold;
	if (bHoldMode && IsOpenKey(Key))
	{
		if (!Wheel->CommitHovered())
		{
			DeactivateWidget();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UGadgetWheelScreen::HandleItemSelected(int32 ItemIndex)
{
	if (AGothamCharacter* Hero = Cast<AGothamCharacter>(GetOwningPlayerPawn()))
	{
		Hero->UseGadget(ItemIndex);
	}
	DeactivateWidget();
}
