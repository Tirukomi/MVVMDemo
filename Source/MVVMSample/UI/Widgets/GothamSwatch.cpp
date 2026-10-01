// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamSwatch.h"

#include "UI/Style/GothamStyle.h"

void UGothamSwatch::SetColorToken(EGothamColorToken InToken, float InAlpha)
{
	Token = InToken;
	Alpha = InAlpha;
	ApplyTheme();
}

void UGothamSwatch::ApplyTheme()
{
	SetBrushColor(GothamStyle::Token(this, Token, Alpha));
}

TSharedRef<SWidget> UGothamSwatch::RebuildWidget()
{
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyTheme(); });
	ApplyTheme();
	return Super::RebuildWidget();
}

void UGothamSwatch::ReleaseSlateResources(bool bReleaseChildren)
{
	SettingsListener.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}
