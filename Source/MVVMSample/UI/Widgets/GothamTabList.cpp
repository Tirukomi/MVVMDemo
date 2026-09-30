// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamTabList.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "UI/GothamWidgetTick.h"
#include "UI/Style/GothamMotion.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamHintButton.h"

UGothamSwitcher::UGothamSwitcher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransitionType = ECommonSwitcherTransition::FadeOnly;
	TransitionCurveType = ETransitionCurve::QuadOut;
	TransitionDuration = GothamMotion::ScreenSeconds;
}

void UGothamTabList::EnsureTree()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = Row;

	// The Q / E (LB / RB) prompts are buttons too: clicking one steps the tab, as the key does.
	auto AddPrompt = [&](const FKey& Keyboard, const FKey& Pad, int32 Direction)
	{
		UGothamHintButton* Prompt = WidgetTree->ConstructWidget<UGothamHintButton>();
		Prompt->SetHint(Keyboard, Pad, FText::GetEmpty());
		Prompt->OnClicked().AddWeakLambda(this, [this, Direction]() { SelectRelative(Direction); });
		Row->AddChildToHorizontalBox(Prompt)->SetVerticalAlignment(VAlign_Center);
	};
	AddPrompt(EKeys::Q, EKeys::Gamepad_LeftShoulder, -1);
	TabBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	Row->AddChildToHorizontalBox(TabBox)->SetPadding(FMargin(10.f, 0.f));
	AddPrompt(EKeys::E, EKeys::Gamepad_RightShoulder, +1);
}

TSharedRef<SWidget> UGothamTabList::RebuildWidget()
{
	EnsureTree();
	return Super::RebuildWidget();
}

void UGothamTabList::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
}

bool UGothamTabList::AddTab(FName TabId, const FText& Label, UWidget* Content)
{
	EnsureTree();
	if (!RegisterTab(TabId, UGothamButton::StaticClass(), Content))
	{
		return false;
	}
	if (UGothamButton* Button = Cast<UGothamButton>(GetTabButtonBaseByID(TabId)))
	{
		Button->SetKind(EGothamButtonKind::Tab);
		Button->SetLabel(Label);
	}
	return true;
}

void UGothamTabList::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	EnsureTree();
	TabBox->AddChildToHorizontalBox(TabButton)->SetPadding(FMargin(2.f, 0.f));
}

void UGothamTabList::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	if (TabBox && TabButton)
	{
		TabBox->RemoveChild(TabButton);
	}
}

bool UGothamTabList::SelectRelative(int32 Direction)
{
	const int32 Count = GetTabCount();
	if (Count < 2)
	{
		return false;
	}
	int32 Current = 0;
	for (int32 i = 0; i < Count; ++i)
	{
		if (GetTabIdAtIndex(i) == GetSelectedTabId())
		{
			Current = i;
			break;
		}
	}
	const int32 Next = ((Current + Direction) % Count + Count) % Count;
	return SelectTabByID(GetTabIdAtIndex(Next));
}

bool UGothamTabList::IsTabKey(const FKey& Key, int32& OutDirection)
{
	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		OutDirection = -1;
		return true;
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		OutDirection = +1;
		return true;
	}
	return false;
}
