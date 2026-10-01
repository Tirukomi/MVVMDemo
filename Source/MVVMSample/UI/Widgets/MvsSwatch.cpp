// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsSwatch.h"

#include "UI/Style/MvsStyle.h"

void UMvsSwatch::SetColorToken(EMvsColorToken InToken, float InAlpha)
{
	Token = InToken;
	Alpha = InAlpha;
	ApplyTheme();
}

void UMvsSwatch::ApplyTheme()
{
	SetBrushColor(MvsStyle::Token(this, Token, Alpha));
}

TSharedRef<SWidget> UMvsSwatch::RebuildWidget()
{
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyTheme(); });
	ApplyTheme();
	return Super::RebuildWidget();
}

void UMvsSwatch::ReleaseSlateResources(bool bReleaseChildren)
{
	SettingsListener.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
}
