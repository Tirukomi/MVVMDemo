// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MVVMSample : ModuleRules
{
	public MVVMSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore",
			"ModelViewViewModel", "FieldNotification",
			"CommonUI", "CommonInput",
			"GameplayTags", "DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
