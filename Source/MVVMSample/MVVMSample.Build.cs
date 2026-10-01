// Copyright IG. All Rights Reserved.

using UnrealBuildTool;

public class MVVMSample : ModuleRules
{
	public MVVMSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Headers live in feature folders (Core/, Gameplay/, ...) and are included as "Folder/File.h". The module root is
		// public on purpose: the one other module, MVVMSampleTests, reaches the game through these headers (exported
		// types only), and a Public/Private split would only move every header into Public.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore",
			"ModelViewViewModel", "FieldNotification",
			"CommonUI", "CommonInput",
			"GameplayTags", "DeveloperSettings"
		});

		// The perf harness (Core/MvsPerfHarness.cpp, compiled out of Shipping) reads the game thread and GPU frame times.
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "RHI" });
		}
	}
}
