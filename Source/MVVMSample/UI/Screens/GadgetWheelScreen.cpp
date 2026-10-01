// Copyright IG. All Rights Reserved.

#include "UI/Screens/GadgetWheelScreen.h"

#include "Gameplay/TimeScaleSubsystem.h"
#include "UI/Style/MvsMotion.h"

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/GadgetWheel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/MvsViewModelSubsystem.h"

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
		Wheel->WheelStyle.LabelFont = MvsStyle::Font(EMvsTextStyle::Header);
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
		if (auto* ViewModels = LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>())
		{
			GadgetBar = ViewModels->GetGadgetBar();
			using FSlotVM = UGadgetSlotViewModel::FFieldNotificationClassDescriptor;
			for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
			{
				MvsMVVM::Bind(SlotVM, this, &UGadgetWheelScreen::OnSlotChanged, { FSlotVM::CooldownPercent, FSlotVM::bIsReady });
			}
		}
	}
	RefreshItems();

	if (!bOpenKeyBound)
	{
		bOpenKeyBound = true;
		const UInputAction* OpenAction = FindGameplayAction(TEXT("GadgetWheel"));
		BindAction(OpenAction, IE_Pressed, FSimpleDelegate::CreateUObject(this, &UGadgetWheelScreen::HandleOpenKey, IE_Pressed));
		BindAction(OpenAction, IE_Released, FSimpleDelegate::CreateUObject(this, &UGadgetWheelScreen::HandleOpenKey, IE_Released));
	}
}

void UGadgetWheelScreen::NativeDestruct()
{
	if (GadgetBar)
	{
		for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
		{
			MvsMVVM::Unbind(SlotVM, this);
		}
	}
	Super::NativeDestruct();
}

void UGadgetWheelScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (UMvsTimeScaleSubsystem* TimeScale = UMvsTimeScaleSubsystem::Get(this))
	{
		TimeScale->Request(TEXT("GadgetWheel"), WheelTimeDilation);
	}
}

void UGadgetWheelScreen::NativeOnClosed()
{
	if (UMvsTimeScaleSubsystem* TimeScale = UMvsTimeScaleSubsystem::Get(this))
	{
		TimeScale->Clear(TEXT("GadgetWheel"));
	}
	Super::NativeOnClosed();
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
	TArray<FMvsWheelItem> Items;
	for (UGadgetSlotViewModel* SlotVM : GadgetBar->GetSlots())
	{
		FMvsWheelItem& Item = Items.AddDefaulted_GetRef();
		Item.Label = SlotVM->GetDisplayName();
		Item.Tint = SlotVM->GetTint();
		Item.bReady = SlotVM->GetIsReady();
		Item.CooldownPercent = SlotVM->GetCooldownPercent();
	}
	Wheel->SetItems(Items);
}

void UGadgetWheelScreen::ApplyTheme()
{
	Super::ApplyTheme();
	// Runs on every settings change, so reduced motion applies to an open wheel too.
	if (Wheel)
	{
		Wheel->bReduceMotion = MvsMotion::IsReduced(this);
		Wheel->SynchronizeProperties();
	}
}

void UGadgetWheelScreen::HandleOpenKey(EInputEvent Event)
{
	// Hold mode commits on release of the key that opened the wheel; toggle mode on the next press.
	const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this);
	const bool bHoldMode = !Settings || Settings->GetSettings().WheelMode == EMvsWheelMode::Hold;
	if ((Event == IE_Released) == bHoldMode && !Wheel->CommitHovered())
	{
		DeactivateWidget();
	}
}

void UGadgetWheelScreen::HandleItemSelected(int32 ItemIndex)
{
	if (GadgetBar)
	{
		GadgetBar->RequestUse(ItemIndex);
	}
	DeactivateWidget();
}
