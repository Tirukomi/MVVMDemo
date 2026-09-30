// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "UI/Widgets/GothamText.h"

#if WITH_DEV_AUTOMATION_TESTS

// Capitals must survive German: "ß" upper-cases to "SS", which Slate's ToUpper transform policy cannot do (it ensures
// on the length change and leaves the text in mixed case).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGothamTextUpperCaseTest, "Gotham.UI.TextUpperCase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter)
bool FGothamTextUpperCaseTest::RunTest(const FString& Parameters)
{
	UGothamText* Text = NewObject<UGothamText>();
	const FText German = FText::FromString(TEXT("Untertitelgröße"));

	Text->SetText(German);
	TestEqual("mixed case by default", Text->GetText().ToString(), German.ToString());

	GothamText::SetUpperCase(Text, true);
	TestEqual("capitals, with ß as SS", Text->GetText().ToString(), FString(TEXT("UNTERTITELGRÖSSE")));
	TestTrue("no Slate transform (it would ensure)", Text->GetTextTransformPolicy() == ETextTransformPolicy::None);

	Text->SetText(FText::FromString(TEXT("Straße")));
	TestEqual("later text is upper-cased too", Text->GetText().ToString(), FString(TEXT("STRASSE")));

	GothamText::SetUpperCase(Text, false);
	TestEqual("switching back restores the original", Text->GetText().ToString(), FString(TEXT("Straße")));
	return true;
}

#endif
