// Copyright IG. All Rights Reserved.

#include "MvsTestDesignerPause.h"

#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "UI/Widgets/MvsText.h"

bool UMvsTestDesignerPause::Initialize()
{
	const bool bOk = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget && !HasAnyFlags(RF_ClassDefaultObject))
	{
		DesignerRoot = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DesignerStatus"));
		WidgetTree->RootWidget = DesignerRoot;
		DesignerLabel = WidgetTree->ConstructWidget<UMvsText>(UMvsText::StaticClass(), TEXT("ObjectiveLabel"));
		DesignerLabel->SetText(NSLOCTEXT("Mvs.PauseMenu", "ObjectiveLabel", "Objective"));
		// What the designer sets in the Details panel, under "Mvs".
		DesignerLabel->Style = EMvsDesignerTextStyle::Label;
		DesignerLabel->Color = EMvsDesignerColor::TextMuted;
		DesignerRoot->AddChildToVerticalBox(DesignerLabel);
	}
	return bOk;
}
