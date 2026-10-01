// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Screens/SettingsScreen.h"

#include "UI/Style/GothamMetrics.h"
#include "Accessibility/GothamSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Widgets/GothamButton.h"
#include "UI/Widgets/GothamMenuList.h"
#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Widgets/GothamPanel.h"
#include "UI/Widgets/GothamTabList.h"
#include "ViewModels/GothamMVVM.h"
#include "ViewModels/SettingsViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.SettingsScreen"

TSharedRef<SWidget> USettingsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Options"), LOCTEXT("Title", "Settings"));

		Tabs = WidgetTree->ConstructWidget<UGothamTabList>();
		Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the pages. Each page scrolls, so large UI scales and long translations never push rows off screen.
		USizeBox* PageWidth = WidgetTree->ConstructWidget<USizeBox>();
		PageWidth->SetWidthOverride(GothamMetrics::SettingsPageWidth);
		Split->AddChildToHorizontalBox(PageWidth);
		Switcher = WidgetTree->ConstructWidget<UGothamSwitcher>();
		PageWidth->SetContent(Switcher);
		Tabs->SetLinkedSwitcher(Switcher);

		for (const FGothamSettingsTab& Tab : USettingsViewModel::GetTabs())
		{
			UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
			// Keep the focused row on screen as gamepad / keyboard focus moves.
			Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
			UGothamMenuList* List = WidgetTree->ConstructWidget<UGothamMenuList>();
			Scroll->AddChild(List);
			List->OnCurrentItemChanged.AddUObject(this, &USettingsScreen::OnCurrentItemChanged);
			Pages.Add(List);

			for (const EGothamSetting Setting : Tab.Settings)
			{
				UGothamOptionRow* Row = WidgetTree->ConstructWidget<UGothamOptionRow>();
				List->AddItem(Row);
				Rows.Add(Row);
				RowSettings.Add(Setting);
			}
			if (Tab.Id == TEXT("Controls"))
			{
				// Rebinding has its own screen; it lives in this tab as one more item.
				UGothamButton* Bindings = AddMenuItem(List, LOCTEXT("KeyBindings", "Key bindings"));
				Bindings->OnClicked().AddLambda([this]() { OpenControls(); });
				KeyBindingsItem = Bindings;
			}
			// Pages must already be in the linked switcher: RegisterTab only records them and switches between them.
			Switcher->AddChild(Scroll);
			Tabs->AddTab(Tab.Id, Tab.Label, Scroll);
		}
		// Every tab lists at least one item (the first tab, Gameplay, has three); fall back to the tab bar if not.
		DefaultFocus = Pages.IsEmpty() || Pages[0]->GetItems().IsEmpty() ? static_cast<UWidget*>(Tabs) : Pages[0]->GetItems()[0].Get();
		Switcher->OnActiveWidgetIndexChanged.AddUObject(this, &USettingsScreen::OnPageShown);

		Split->AddChildToHorizontalBox(WidgetTree->ConstructWidget<USpacer>())->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Right: what the focused option does.
		USizeBox* DetailWidth = WidgetTree->ConstructWidget<USizeBox>();
		DetailWidth->SetWidthOverride(GothamMetrics::SettingsDetailWidth);
		Split->AddChildToHorizontalBox(DetailWidth)->SetVerticalAlignment(VAlign_Top);
		DetailPanel = WidgetTree->ConstructWidget<UGothamPanel>();
		DetailPanel->SetPanelPadding(GothamMetrics::SettingsDetailPadding);
		DetailPanel->SetShape(12.f, EGothamChamfer::Opposite);
		DetailWidth->SetContent(DetailPanel);
		UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailPanel->SetContent(Detail);
		DetailTitle = MakeText(FText::GetEmpty(), EGothamTextStyle::Header, EGothamColorToken::TextPrimary);
		DetailTitle->SetAutoWrapText(true);
		Detail->AddChildToVerticalBox(DetailTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		DetailBody = MakeText(FText::GetEmpty(), EGothamTextStyle::Body, EGothamColorToken::TextMuted);
		DetailBody->SetAutoWrapText(true);
		// Long compounds (German) and unbroken runs (pseudo-locale) wrap by character rather than overflow.
		DetailBody->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		Detail->AddChildToVerticalBox(DetailBody);

		// Bottom-left: unapplied-changes note and the actions.
		DirtyNote = MakeText(FText::GetEmpty(), EGothamTextStyle::Label, EGothamColorToken::Warning);
		Column->AddChildToVerticalBox(DirtyNote)->SetPadding(FMargin(GothamMetrics::ItemIndent, 12.f, 0.f, 8.f));

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons);
		auto AddAction = [&](const FText& Label, TFunction<void()> Action)
		{
			UGothamButton* Button = WidgetTree->ConstructWidget<UGothamButton>();
			Button->SetLabel(Label);
			Button->OnClicked().AddLambda(MoveTemp(Action));
			Buttons->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, GothamMetrics::ButtonGap, 0.f));
		};
		AddAction(LOCTEXT("Apply", "Apply"), [this]() { if (ViewModel) { ViewModel->Apply(); } });
		AddAction(LOCTEXT("Revert", "Revert"), [this]() { if (ViewModel) { ViewModel->Revert(); } });
		AddAction(LOCTEXT("Defaults", "Defaults"), [this]() { if (ViewModel) { ViewModel->ResetDefaults(); } });
		AddAction(LOCTEXT("Back", "Back"), [this]() { DeactivateWidget(); });

		AddFooter(MakeActionBar(LOCTEXT("Select", "Change"), LOCTEXT("BackHint", "Back (discards unapplied changes)")));
	}
	return Super::RebuildWidget();
}

void USettingsScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		ViewModel = Settings->GetViewModel();
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			Rows[i]->Setup(RowSettings[i], ViewModel);
		}
		using FVM = USettingsViewModel::FFieldNotificationClassDescriptor;
		GothamMVVM::Bind(ViewModel, this, &USettingsScreen::OnViewModelChanged, { FVM::bIsDirty, FVM::Revision });
	}
	RefreshDirtyNote();
	ShowDetail(Rows.IsEmpty() ? nullptr : Rows[0].Get());

#if !UE_BUILD_SHIPPING
	// Screenshot aid: -GothamSettingsTab=<Id> opens on that tab, focused on its first item (or on item
	// -GothamSettingsItem=<n>, counted from 0).
	FString StartTab;
	if (FParse::Value(FCommandLine::Get(), TEXT("GothamSettingsTab="), StartTab) && Tabs->SelectTabByID(FName(*StartTab)))
	{
		const int32 Index = USettingsViewModel::GetTabs().IndexOfByPredicate([&StartTab](const FGothamSettingsTab& Tab) { return Tab.Id == FName(*StartTab); });
		int32 Item = 0;
		FParse::Value(FCommandLine::Get(), TEXT("GothamSettingsItem="), Item);
		if (Pages.IsValidIndex(Index) && Pages[Index]->GetItems().IsValidIndex(Item))
		{
			DefaultFocus = Pages[Index]->GetItems()[Item];
			ShowDetail(DefaultFocus);
		}
	}
#endif
}

void USettingsScreen::NativeDestruct()
{
	GothamMVVM::Unbind(ViewModel, this);
	Super::NativeDestruct();
}

UWidget* USettingsScreen::NativeGetDesiredFocusTarget() const
{
	// The screen is reused and keeps its tab, so focus must land on the page that is showing: a row on a hidden page
	// cannot take focus, and focus would stay on the game viewport.
	const int32 Index = Switcher ? Switcher->GetActiveWidgetIndex() : 0;
	if (Pages.IsValidIndex(Index) && !Pages[Index]->GetItems().IsEmpty())
	{
		const TArray<TObjectPtr<UWidget>>& Items = Pages[Index]->GetItems();
		return Items.Contains(DefaultFocus) ? DefaultFocus.Get() : Items[0].Get();
	}
	return Super::NativeGetDesiredFocusTarget();
}

void USettingsScreen::NativeOnClosed()
{
	// Leaving with unapplied changes discards them, so a previewed language or scale never sticks by accident.
	// Only when leaving: Key bindings opens on top of this screen and must not throw the changes away.
	if (ViewModel)
	{
		ViewModel->Revert();
	}
	Super::NativeOnClosed();
}

void USettingsScreen::OnPageShown(UWidget* Page, int32 Index)
{
	// Focus follows the tab: land on the first item of the page that just became active.
	if (IsActivated() && Pages.IsValidIndex(Index) && !Pages[Index]->GetItems().IsEmpty())
	{
		UWidget* First = Pages[Index]->GetItems()[0];
		First->SetFocus();
		ShowDetail(First);
	}
}

void USettingsScreen::OnCurrentItemChanged(UWidget* Item)
{
	ShowDetail(Item);
}

void USettingsScreen::ShowDetail(UWidget* Item)
{
	DetailItem = Item;
	if (!DetailTitle)
	{
		return;
	}
	if (const UGothamOptionRow* Row = Cast<UGothamOptionRow>(Item))
	{
		DetailTitle->SetText(USettingsViewModel::GetLabel(Row->GetSetting()));
		DetailBody->SetText(USettingsViewModel::GetDescription(Row->GetSetting()));
	}
	else if (Item && Item == KeyBindingsItem)
	{
		DetailTitle->SetText(LOCTEXT("KeyBindings", "Key bindings"));
		DetailBody->SetText(LOCTEXT("KeyBindingsDesc", "Change the key or button for every action, separately for keyboard and mouse and for gamepad."));
	}
}

void USettingsScreen::OnViewModelChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	RefreshDirtyNote();
	// A language change (a Revision bump) re-texts the detail pane too.
	ShowDetail(DetailItem.Get());
}

void USettingsScreen::ApplyTheme()
{
	Super::ApplyTheme();
	if (DetailPanel)
	{
		DetailPanel->SetColors(GothamStyle::Token(this, EGothamColorToken::Panel, GothamStyle::PanelAlpha(this)),
			GothamStyle::Token(this, EGothamColorToken::PanelEdge, 0.7f));
		DetailPanel->SetAccent(GothamStyle::Token(this, EGothamColorToken::Accent), 3.f);
	}
}

void USettingsScreen::RefreshDirtyNote()
{
	if (!DirtyNote)
	{
		return;
	}
	const bool bDirty = ViewModel && ViewModel->GetIsDirty();
	DirtyNote->SetText(bDirty ? LOCTEXT("Unapplied", "Unapplied changes. Choose Apply to keep them.") : FText::GetEmpty());
}

void USettingsScreen::OpenControls()
{
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UGothamUISubsystem>())
	{
		UI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->ControlsScreenClass);
	}
}

#undef LOCTEXT_NAMESPACE
