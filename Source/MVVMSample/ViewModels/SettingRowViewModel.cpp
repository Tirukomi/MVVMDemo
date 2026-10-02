// Copyright IG. All Rights Reserved.

#include "ViewModels/SettingRowViewModel.h"

#include "Accessibility/MvsSettingsTable.h"
#include "ViewModels/SettingsViewModel.h"

void USettingRowViewModel::Initialize(USettingsViewModel* InOwner, EMvsSetting InSetting)
{
	Owner = InOwner;
	Setting = InSetting;
	bWraps = Setting != EMvsSetting::UIScale;
}

void USettingRowViewModel::Update(const FMvsSettingsData& Data, bool bRetext)
{
	if (Setting >= EMvsSetting::Count)
	{
		return;
	}
	const FMvsSettingDescriptor& Def = MvsSettingsTable::Find(Setting);
	SetText(Label, Def.Label, FFieldNotificationClassDescriptor::Label, bRetext);
	SetText(Description, Def.Description, FFieldNotificationClassDescriptor::Description, bRetext);
	SetText(ValueText, Def.FormatValue(Data), FFieldNotificationClassDescriptor::ValueText, bRetext);
	int32 Index = 0;
	int32 Count = 1;
	Data.GetOptionPosition(Setting, Index, Count);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(ChoiceIndex, Index);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(ChoiceCount, Count);
}

void USettingRowViewModel::Step(int32 Direction)
{
	if (USettingsViewModel* Settings = Owner.Get())
	{
		Settings->Cycle(Setting, Direction);
	}
}

void USettingRowViewModel::SetText(FText& Field, const FText& NewValue, UE::FieldNotification::FFieldId FieldId, bool bForce)
{
	// Formatted values are new FText objects every time, so "identical" would notify on every update; what matters is
	// whether the row would read differently.
	if (bForce || !Field.ToString().Equals(NewValue.ToString(), ESearchCase::CaseSensitive))
	{
		Field = NewValue;
		BroadcastFieldValueChanged(FieldId);
	}
}
