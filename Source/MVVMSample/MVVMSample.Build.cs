// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MVVMSample : ModuleRules
{
	public MVVMSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Headers live in feature folders (Core/, Gameplay/, ...) and are included as "Folder/File.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore",
			"ModelViewViewModel", "FieldNotification",
			"CommonUI", "CommonInput",
			"GameplayTags", "DeveloperSettings", "RenderCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
