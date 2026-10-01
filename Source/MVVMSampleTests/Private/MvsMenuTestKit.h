// Copyright IG. All Rights Reserved.

#pragma once

// What the functional menu tests share: a latent command that runs a script once the game is up, the player's UI and
// view models, and input driven through Slate's own input path (in-engine key and mouse events, never OS input).
// The tests cover what unit tests cannot: focus, hit-testing and routing. One test per screen (second review 17), so
// one bug fails one test instead of every rule after it.

#include "CoreMinimal.h"
#include "Core/MvsScript.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectIterator.h"

#if WITH_DEV_AUTOMATION_TESTS

class AMvsPlayerController;
class UCommonActivatableWidget;
class UMvsHintButton;
class UMvsOptionRow;
class UMvsUISettings;
class UMvsUISubsystem;
class UMvsViewModelSubsystem;
class USettingsViewModel;
class UUserWidget;
class UWidget;
enum class EMvsSetting : uint8;

namespace MvsMenuTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
	/** How long a screen may take to open and settle, and how long anything else may take. */
	constexpr float Open = 3.f;
	constexpr float Quick = 1.5f;

	/**
	 * The player's UI, view models and settings, and the script's checks. Steps capture it by value: it holds weak
	 * pointers, and the script as a raw pointer (the ticker owns the script; steps holding a shared ref would form a cycle).
	 */
	struct FRig
	{
		FMvsScript* Self = nullptr;
		TWeakObjectPtr<UMvsUISubsystem> UI;
		TWeakObjectPtr<AMvsPlayerController> PC;

		void Check(bool bPassed, const FString& Rule) const;
		UMvsViewModelSubsystem* ViewModels() const;
		USettingsViewModel* Settings() const;
		/** The UI scale option's index, or -1 without settings. */
		int32 Scale() const;
		/** Pushes a screen class from the UI settings onto the menu layer. */
		FMvsScript::FAction Push(TSoftClassPtr<UCommonActivatableWidget> UMvsUISettings::* Class) const;
		/** A screen is ready for input once it is active, no layer is mid-transition (Common UI blocks input meanwhile),
		 *  and focus has landed inside it. */
		bool Settled(const UUserWidget* Screen) const;
		bool Closed(const UUserWidget* Screen) const;
	};

	/** Adds a test's rules to a script that starts and ends with no menu open and the settings reverted. */
	using FBody = void (*)(FMvsScript& Script, const FRig& Rig);

	/**
	 * Runs Body as the current automation test: waits for the level and the player, settles once per game session (the
	 * HUD's intro), and finishes with the script. Every rule becomes a test pass (info) or error.
	 */
	void Run(FAutomationTestBase* Test, const TCHAR* Name, FBody Body);

	template<typename T>
	T* ActiveScreen()
	{
		for (TObjectIterator<T> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
			{
				return *It;
			}
		}
		return nullptr;
	}

	/** The first T inside Screen. */
	template<typename T>
	T* FindIn(const UObject* Screen)
	{
		for (TObjectIterator<T> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && Screen && It->IsIn(Screen))
			{
				return *It;
			}
		}
		return nullptr;
	}

	void SendKey(const FKey& Key);
	void ReleaseKey(const FKey& Key);
	/** A mouse move that travels (Common Input counts that as switching to the mouse). */
	void MoveMouse();

	/**
	 * A left click the way a mouse makes one, through Slate's hit-testing: the pointer arrives, the button goes down on
	 * the next frame and up on the frame after. (Move, press and release in one instant is not something real input
	 * does, and Common UI buttons then miss the click now and then.) Fraction picks the point inside Target.
	 */
	void AddClick(FMvsScript& Script, TFunction<const UWidget*()> Target, const FString& Rule, TFunction<FVector2D(const FGeometry&)> Fraction = nullptr);

	/**
	 * A condition that holds once Get's widget has kept the same on-screen position and size for two frames in a row:
	 * layout changes (UI scale), animated scrolling and sliding highlights take a few frames to settle, and a click
	 * aimed before that misses.
	 */
	TFunction<bool()> Stable(TFunction<const UWidget*()> Get);

	/**
	 * The prompt showing this keyboard key inside Screen: one the screen's action bar is showing (the bar pools the
	 * prompts it replaces, so only its current entries count), or one of the tab list's.
	 */
	UMvsHintButton* FindHint(const UObject* Screen, const FKey& Key);
	/** The text of Hint's key cap. */
	FString GlyphText(const UMvsHintButton* Hint);
	/** The label colour of the [Esc] prompt in Screen (transparent if not found). */
	FLinearColor PromptLabelColor(const UObject* Screen);
	/** The settings row for one option, inside Screen. */
	UMvsOptionRow* FindRow(const UObject* Screen, EMvsSetting Setting);
	FName SelectedTab(const UObject* Screen);

	/** What a screen reader would say for the focused widget. */
	FString FocusedText();
	/** True if the user's keyboard focus is Screen's widget or inside it. */
	bool HasFocusWithin(const UUserWidget* Screen);

	/** Rebinds Action's keyboard slot in memory (never saved), remembering the key it had in Undo. */
	void Rebind(const TWeakObjectPtr<AMvsPlayerController>& PC, FName Action, const FKey& Key, TArray<TPair<FName, FKey>>& Undo);
	/** Puts back exactly the keys Rebind replaced, so the player's own rebinds are untouched. */
	void RestoreBindings(const TWeakObjectPtr<AMvsPlayerController>& PC, TArray<TPair<FName, FKey>>& Undo);
}

#endif
