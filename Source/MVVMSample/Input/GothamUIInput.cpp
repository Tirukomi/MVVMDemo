// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/GothamUIInput.h"

#include "InputAction.h"
#include "InputMappingContext.h"

#define LOCTEXT_NAMESPACE "Gotham.UIInput"

namespace
{
	UInputAction* MakeMenuAction(UObject* Owner, const TCHAR* Name, const FText& Description)
	{
		UInputAction* Action = Owner->CreateDefaultSubobject<UInputAction>(Name);
		Action->ActionDescription = Description;
		Action->bConsumeInput = false;
		return Action;
	}
}

UGothamUIInputData::UGothamUIInputData()
{
	EnhancedInputClickAction = MakeMenuAction(this, TEXT("IA_UI_Accept"), LOCTEXT("Accept", "Select"));
	EnhancedInputBackAction = MakeMenuAction(this, TEXT("IA_UI_Back"), LOCTEXT("Back", "Back"));
	PreviousTabAction = MakeMenuAction(this, TEXT("IA_UI_PreviousTab"), LOCTEXT("PreviousTab", "Previous tab"));
	NextTabAction = MakeMenuAction(this, TEXT("IA_UI_NextTab"), LOCTEXT("NextTab", "Next tab"));
}

UInputMappingContext* UGothamUIInputData::BuildMappingContext(UObject* Outer) const
{
	UInputMappingContext* Context = NewObject<UInputMappingContext>(Outer, TEXT("IMC_Menu"));
	// Keyboard keys first: prompts show the first key of the current device.
	Context->MapKey(EnhancedInputClickAction, EKeys::Enter);
	Context->MapKey(EnhancedInputClickAction, EKeys::Virtual_Gamepad_Accept.GetVirtualKey());
	Context->MapKey(EnhancedInputBackAction, EKeys::Escape);
	Context->MapKey(EnhancedInputBackAction, EKeys::Virtual_Gamepad_Back.GetVirtualKey());
	Context->MapKey(PreviousTabAction, EKeys::Q);
	Context->MapKey(PreviousTabAction, EKeys::Gamepad_LeftShoulder);
	Context->MapKey(NextTabAction, EKeys::E);
	Context->MapKey(NextTabAction, EKeys::Gamepad_RightShoulder);
	return Context;
}

#undef LOCTEXT_NAMESPACE
