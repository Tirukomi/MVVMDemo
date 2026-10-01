// Copyright IG. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class MVVMSampleEditorTarget : TargetRules
{
	public MVVMSampleEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MVVMSample");
		ExtraModuleNames.Add("MVVMSampleTests");
	}
}
