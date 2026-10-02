// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Margin.h"

/**
 * The layout metrics of the menus, in Slate units at 1x UI scale. Code-built screens use these names instead of
 * literals, so a spacing or width is changed in one place and tests read the same numbers the layout uses.
 */
namespace MvsMetrics
{
	// --- Menu frame (UMvsScreen::BuildMenuFrame) ---------------------------------------------------------------
	/** Space between the screen edge (inside the safe zone) and the menu column. */
	inline const FMargin FrameMargin(96.f, 64.f, 96.f, 48.f);
	/** Below the section label, around the title. */
	inline const FMargin TitlePadding(0.f, 2.f, 0.f, 10.f);
	/** Below the header rule, before the content. */
	inline constexpr float HeaderRuleGap = 26.f;
	/** The accent bar at the start of the header rule. */
	inline constexpr float AccentBarWidth = 64.f;
	inline constexpr float AccentBarHeight = 3.f;
	/** Above the action bar. */
	inline constexpr float FooterGap = 16.f;
	/** Between prompts in the action bar. */
	inline constexpr float PromptSpacing = 20.f;

	// --- Rows and items -------------------------------------------------------------------------------------------
	/** Left indent of list rows and their captions, so labels line up under the highlight bar's accent. */
	inline constexpr float ItemIndent = 22.f;
	/** Between buttons in a row (Apply / Revert / Defaults, Reset / Back). */
	inline constexpr float ButtonGap = 10.f;
	/** A settings row: indent on the left, the selector's inset on the right. */
	inline constexpr float SelectorInset = 14.f;
	inline const FMargin OptionRowPadding(ItemIndent, 6.f, SelectorInset, 6.f);
	/** The value selector at the right of a settings row (chevrons, value, pips). */
	inline constexpr float SelectorWidth = 260.f;

	// --- Buttons (UMvsButton) -------------------------------------------------------------------------------------
	/** A big menu item: indented like list rows, room on the right for the highlight bar's tail. */
	inline const FMargin MenuItemPadding(ItemIndent, 9.f, 40.f, 9.f);
	/** A tab and the size of its chamfered corners. */
	inline const FMargin TabPadding(20.f, 8.f);
	inline constexpr float TabCorner = 6.f;
	/** A standard or danger button and its corners. */
	inline const FMargin StandardButtonPadding(ItemIndent, 9.f);
	inline constexpr float StandardButtonCorner = 8.f;

	// --- Columns and panels ---------------------------------------------------------------------------------------
	inline constexpr float SettingsPageWidth = 660.f;
	inline constexpr float SettingsDetailWidth = 400.f;
	inline const FMargin SettingsDetailPadding(22.f, 16.f, 22.f, 20.f);
	inline constexpr float CaseFileDetailWidth = 420.f;
	inline const FMargin CaseFileDetailPadding(18.f, 18.f, 18.f, 20.f);
	inline constexpr float PauseStatusWidth = 360.f;
	inline const FMargin PauseStatusPadding(22.f, 16.f, 22.f, 18.f);
	inline constexpr float ModalWidth = 540.f;
	inline const FMargin ModalPadding(34.f, 24.f, 30.f, 24.f);
}
