// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamInputGlyph.h"

#include "Blueprint/WidgetTree.h"
#include "CommonInputSubsystem.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"

TSharedRef<SWidget> UGothamInputGlyph::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UBorder* Frame = WidgetTree->ConstructWidget<UBorder>();
		Frame->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.18f));
		Frame->SetPadding(FMargin(8.f, 2.f));
		WidgetTree->RootWidget = Frame;

		Text = WidgetTree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = 14;
		Text->SetFont(Font);
		Frame->SetContent(Text);
	}
	return Super::RebuildWidget();
}

void UGothamInputGlyph::NativeConstruct()
{
	Super::NativeConstruct();
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		InputMethodHandle = Input->OnInputMethodChangedNative.AddUObject(this, &UGothamInputGlyph::HandleInputMethodChanged);
	}
	Refresh();
}

void UGothamInputGlyph::NativeDestruct()
{
	if (UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer()))
	{
		Input->OnInputMethodChangedNative.Remove(InputMethodHandle);
	}
	FTSTicker::GetCoreTicker().RemoveTicker(RetryHandle);
	Super::NativeDestruct();
}

void UGothamInputGlyph::SetAction(FName InActionName)
{
	ActionName = InActionName;
	RetriesLeft = 10;
	Refresh();
}

void UGothamInputGlyph::SetFixedKeys(FKey InKeyboardMouseKey, FKey InGamepadKey)
{
	FixedKeyboardMouse = InKeyboardMouseKey;
	FixedGamepad = InGamepadKey;
	Refresh();
}

FText UGothamInputGlyph::GetKeyLabel(const FKey& Key)
{
	static const TMap<FName, const TCHAR*> ShortNames = {
		{ EKeys::Gamepad_FaceButton_Bottom.GetFName(), TEXT("A") },
		{ EKeys::Gamepad_FaceButton_Right.GetFName(), TEXT("B") },
		{ EKeys::Gamepad_FaceButton_Left.GetFName(), TEXT("X") },
		{ EKeys::Gamepad_FaceButton_Top.GetFName(), TEXT("Y") },
		{ EKeys::Gamepad_Special_Right.GetFName(), TEXT("Menu") },
		{ EKeys::Gamepad_RightTrigger.GetFName(), TEXT("RT") },
		{ EKeys::Gamepad_LeftTrigger.GetFName(), TEXT("LT") },
		{ EKeys::Escape.GetFName(), TEXT("Esc") },
		{ EKeys::Enter.GetFName(), TEXT("Enter") },
		{ EKeys::LeftMouseButton.GetFName(), TEXT("LMB") },
	};
	if (const TCHAR* const* Short = ShortNames.Find(Key.GetFName()))
	{
		return FText::FromString(*Short);
	}
	return Key.GetDisplayName();
}

void UGothamInputGlyph::Refresh()
{
	if (!Text)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(LocalPlayer);
	const bool bGamepad = Input && Input->GetCurrentInputType() == ECommonInputType::Gamepad;

	FKey Key = bGamepad ? FixedGamepad : FixedKeyboardMouse;
	if (!ActionName.IsNone())
	{
		Key = EKeys::Invalid;
		const AGothamPlayerController* PC = Cast<AGothamPlayerController>(GetOwningPlayer());
		const UInputAction* Action = PC ? PC->FindAction(ActionName) : nullptr;
		auto* Enhanced = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		if (Action && Enhanced)
		{
			for (const FKey& Candidate : Enhanced->QueryKeysMappedToAction(Action))
			{
				if (Candidate.IsGamepadKey() == bGamepad)
				{
					Key = Candidate;
					break;
				}
			}
		}
	}
	Text->SetText(Key.IsValid() ? GetKeyLabel(Key) : FText::GetEmpty());

	if (!Key.IsValid() && !ActionName.IsNone())
	{
		ScheduleRetry();
	}
}

void UGothamInputGlyph::ScheduleRetry()
{
	FTSTicker::GetCoreTicker().RemoveTicker(RetryHandle);
	if (RetriesLeft <= 0)
	{
		return;
	}
	RetryHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this](float)
	{
		--RetriesLeft;
		Refresh();
		return false;
	}), 0.1f);
}
