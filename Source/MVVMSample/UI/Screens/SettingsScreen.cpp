// Copyright IG. All Rights Reserved.

#include "UI/Screens/SettingsScreen.h"

#include "UI/Style/MvsMetrics.h"
#include "Accessibility/MvsSettingsSubsystem.h"
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
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsMenuList.h"
#include "UI/Widgets/MvsOptionRow.h"
#include "UI/Widgets/MvsPanel.h"
#include "UI/Widgets/MvsTabList.h"
#include "ViewModels/MvsMVVM.h"
#include "ViewModels/SettingRowViewModel.h"
#include "ViewModels/SettingsViewModel.h"

#define LOCTEXT_NAMESPACE "Mvs.SettingsScreen"

TSharedRef<SWidget> USettingsScreen::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Column = BuildMenuFrame(LOCTEXT("Section", "Options"), LOCTEXT("Title", "Settings"));

		Tabs = WidgetTree->ConstructWidget<UMvsTabList>();
		Column->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));

		UHorizontalBox* Split = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Split)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		// Left: the pages. Each page scrolls, so large UI scales and long translations never push rows off screen.
		USizeBox* PageWidth = WidgetTree->ConstructWidget<USizeBox>();
		PageWidth->SetWidthOverride(MvsMetrics::SettingsPageWidth);
		Split->AddChildToHorizontalBox(PageWidth);
		Switcher = WidgetTree->ConstructWidget<UMvsSwitcher>();
		PageWidth->SetContent(Switcher);
		Tabs->SetLinkedSwitcher(Switcher);

		for (const FMvsSettingsTab& Tab : USettingsViewModel::GetTabs())
		{
			UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
			// Keep the focused row on screen as gamepad / keyboard focus moves.
			Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::AnimatedScroll);
			UMvsMenuList* List = WidgetTree->ConstructWidget<UMvsMenuList>();
			Scroll->AddChild(List);
			List->OnCurrentItemChanged.AddUObject(this, &USettingsScreen::OnCurrentItemChanged);
			Pages.Add(List);

			for (const EMvsSetting Setting : Tab.Settings)
			{
				UMvsOptionRow* Row = WidgetTree->ConstructWidget<UMvsOptionRow>();
				List->AddItem(Row);
				Rows.Add(Row);
				RowSettings.Add(Setting);
			}
			if (Tab.Id == TEXT("Controls"))
			{
				// Rebinding has its own screen; it lives in this tab as one more item.
				UMvsButton* Bindings = AddMenuItem(List, LOCTEXT("KeyBindings", "Key bindings"));
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
		DetailWidth->SetWidthOverride(MvsMetrics::SettingsDetailWidth);
		Split->AddChildToHorizontalBox(DetailWidth)->SetVerticalAlignment(VAlign_Top);
		DetailPanel = WidgetTree->ConstructWidget<UMvsPanel>();
		DetailPanel->SetPanelPadding(MvsMetrics::SettingsDetailPadding);
		DetailPanel->SetShape(12.f, EMvsChamfer::Opposite);
		DetailWidth->SetContent(DetailPanel);
		UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>();
		DetailPanel->SetContent(Detail);
		DetailTitle = MakeText(FText::GetEmpty(), EMvsTextStyle::Header, EMvsColorToken::TextPrimary);
		DetailTitle->SetAutoWrapText(true);
		Detail->AddChildToVerticalBox(DetailTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		DetailBody = MakeText(FText::GetEmpty(), EMvsTextStyle::Body, EMvsColorToken::TextMuted);
		DetailBody->SetAutoWrapText(true);
		// Long compounds (German) and unbroken runs (pseudo-locale) wrap by character rather than overflow.
		DetailBody->SetWrappingPolicy(ETextWrappingPolicy::AllowPerCharacterWrapping);
		Detail->AddChildToVerticalBox(DetailBody);

		// Bottom-left: unapplied-changes note and the actions.
		DirtyNote = MakeText(FText::GetEmpty(), EMvsTextStyle::Label, EMvsColorToken::Warning);
		Column->AddChildToVerticalBox(DirtyNote)->SetPadding(FMargin(MvsMetrics::ItemIndent, 12.f, 0.f, 8.f));

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>();
		Column->AddChildToVerticalBox(Buttons);
		auto AddAction = [&](const FText& Label, TFunction<void()> Action)
		{
			UMvsButton* Button = WidgetTree->ConstructWidget<UMvsButton>();
			Button->SetLabel(Label);
			Button->OnClicked().AddLambda(MoveTemp(Action));
			Buttons->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, MvsMetrics::ButtonGap, 0.f));
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

	if (UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this))
	{
		const TWeakObjectPtr<UMvsSettingsSubsystem> WeakSettings(Settings);
		if (!ViewModel)
		{
			// The screen's own view model edits a working copy; the subsystem applies and saves it.
			ViewModel = NewObject<USettingsViewModel>(this);
			ViewModel->OnPreview.AddWeakLambda(this, [WeakSettings](const FMvsSettingsData& Data) { if (WeakSettings.IsValid()) { WeakSettings->Preview(Data); } });
			ViewModel->OnCommitted.AddWeakLambda(this, [WeakSettings](const FMvsSettingsData& Data) { if (WeakSettings.IsValid()) { WeakSettings->Commit(Data); } });
		}
		ViewModel->Sync(Settings->GetSaved(), Settings->GetSettings());
		ModelListener.Bind(Settings, this, [this, WeakSettings](const FMvsSettingsData&)
		{
			if (WeakSettings.IsValid() && ViewModel)
			{
				ViewModel->Sync(WeakSettings->GetSaved(), WeakSettings->GetSettings());
				// A language switch re-texts the detail pane too.
				ShowDetail(DetailItem.Get());
			}
		});
		for (int32 i = 0; i < Rows.Num(); ++i)
		{
			Rows[i]->Setup(ViewModel->GetRow(RowSettings[i]));
		}
		// Each row binds its own option; the screen only needs "unapplied changes".
		using FVM = USettingsViewModel::FFieldNotificationClassDescriptor;
		MvsMVVM::Bind(ViewModel, this, &USettingsScreen::OnViewModelChanged, { FVM::bIsDirty });
	}
	RefreshDirtyNote();
	ShowDetail(Rows.IsEmpty() ? nullptr : Rows[0].Get());

#if !UE_BUILD_SHIPPING
	// Screenshot aid: -MvsSettingsTab=<Id> opens on that tab, focused on its first item (or on item
	// -MvsSettingsItem=<n>, counted from 0).
	FString StartTab;
	if (FParse::Value(FCommandLine::Get(), TEXT("MvsSettingsTab="), StartTab) && Tabs->SelectTabByID(FName(*StartTab)))
	{
		const int32 Index = USettingsViewModel::GetTabs().IndexOfByPredicate([&StartTab](const FMvsSettingsTab& Tab) { return Tab.Id == FName(*StartTab); });
		int32 Item = 0;
		FParse::Value(FCommandLine::Get(), TEXT("MvsSettingsItem="), Item);
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
	ModelListener.Reset();
	MvsMVVM::Unbind(ViewModel, this);
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
	const UMvsOptionRow* Row = Cast<UMvsOptionRow>(Item);
	if (const USettingRowViewModel* RowVM = Row ? Row->GetRow() : nullptr)
	{
		DetailTitle->SetText(RowVM->GetLabel());
		DetailBody->SetText(RowVM->GetDescription());
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
}

void USettingsScreen::ApplyTheme(const FMvsTheme& Theme)
{
	Super::ApplyTheme(Theme);
	if (DetailPanel)
	{
		DetailPanel->SetColors(Theme.Color(EMvsColorToken::Panel, Theme.PanelAlpha()),
			Theme.Color(EMvsColorToken::PanelEdge, 0.7f));
		DetailPanel->SetAccent(Theme.Color(EMvsColorToken::Accent), 3.f);
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
	if (auto* UI = GetOwningLocalPlayer()->GetSubsystem<UMvsUISubsystem>())
	{
		UI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->ControlsScreenClass);
	}
}

#undef LOCTEXT_NAMESPACE
