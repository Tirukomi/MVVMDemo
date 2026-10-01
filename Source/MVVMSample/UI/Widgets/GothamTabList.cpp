// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamTabList.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "Input/GothamUIInput.h"
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

UGothamTabList::UGothamTabList(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoListenForInput = true;
}

void UGothamTabList::EnsureTree()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}
	// Set before construction, which starts listening for them.
	PreviousTabEnhancedInputAction = UGothamUIInputData::Get().GetPreviousTabAction();
	NextTabEnhancedInputAction = UGothamUIInputData::Get().GetNextTabAction();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = Row;

	// The previous / next prompts are buttons too: clicking one steps the tab, as the key does.
	auto AddPrompt = [&](const UInputAction* Action, int32 Direction)
	{
		UGothamHintButton* Prompt = WidgetTree->ConstructWidget<UGothamHintButton>();
		Prompt->SetInputAction(Action, FText::GetEmpty());
		Prompt->OnClicked().AddWeakLambda(this, [this, Direction]() { SelectRelative(Direction); });
		Row->AddChildToHorizontalBox(Prompt)->SetVerticalAlignment(VAlign_Center);
	};
	AddPrompt(PreviousTabEnhancedInputAction, -1);
	TabBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	Row->AddChildToHorizontalBox(TabBox)->SetPadding(FMargin(10.f, 0.f));
	AddPrompt(NextTabEnhancedInputAction, +1);
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
