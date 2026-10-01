// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsTabList.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Slate/SCommonAnimatedSwitcher.h"
#include "Input/MvsUIInput.h"
#include "UI/MvsWidgetTick.h"
#include "UI/Style/MvsMotion.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsHintButton.h"

UMvsSwitcher::UMvsSwitcher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TransitionType = ECommonSwitcherTransition::FadeOnly;
	TransitionCurveType = ETransitionCurve::QuadOut;
	TransitionDuration = MvsMotion::ScreenSeconds;
}

UMvsTabList::UMvsTabList(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoListenForInput = true;
}

void UMvsTabList::EnsureTree()
{
	if (WidgetTree->RootWidget)
	{
		return;
	}
	// Set before construction, which starts listening for them.
	PreviousTabEnhancedInputAction = UMvsUIInputData::Get().GetPreviousTabAction();
	NextTabEnhancedInputAction = UMvsUIInputData::Get().GetNextTabAction();
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = Row;

	// The previous / next prompts are buttons too: clicking one steps the tab, as the key does.
	auto AddPrompt = [&](const UInputAction* Action, int32 Direction)
	{
		UMvsHintButton* Prompt = WidgetTree->ConstructWidget<UMvsHintButton>();
		Prompt->SetInputAction(Action, FText::GetEmpty());
		Prompt->OnClicked().AddWeakLambda(this, [this, Direction]() { SelectRelative(Direction); });
		Row->AddChildToHorizontalBox(Prompt)->SetVerticalAlignment(VAlign_Center);
	};
	AddPrompt(PreviousTabEnhancedInputAction, -1);
	TabBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	Row->AddChildToHorizontalBox(TabBox)->SetPadding(FMargin(10.f, 0.f));
	AddPrompt(NextTabEnhancedInputAction, +1);
}

TSharedRef<SWidget> UMvsTabList::RebuildWidget()
{
	EnsureTree();
	return Super::RebuildWidget();
}

void UMvsTabList::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
}

bool UMvsTabList::AddTab(FName TabId, const FText& Label, UWidget* Content)
{
	EnsureTree();
	if (!RegisterTab(TabId, UMvsButton::StaticClass(), Content))
	{
		return false;
	}
	if (UMvsButton* Button = Cast<UMvsButton>(GetTabButtonBaseByID(TabId)))
	{
		Button->SetKind(EMvsButtonKind::Tab);
		Button->SetLabel(Label);
	}
	return true;
}

void UMvsTabList::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	EnsureTree();
	TabBox->AddChildToHorizontalBox(TabButton)->SetPadding(FMargin(2.f, 0.f));
}

void UMvsTabList::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	if (TabBox && TabButton)
	{
		TabBox->RemoveChild(TabButton);
	}
}

bool UMvsTabList::SelectRelative(int32 Direction)
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
