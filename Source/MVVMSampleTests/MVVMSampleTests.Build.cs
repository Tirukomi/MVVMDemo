// Copyright IG. All Rights Reserved.

using UnrealBuildTool;

/**
 * The automation tests (Mvs.*), apart from the game: a DeveloperTool module, so it is built and loaded in the editor
 * and in Development game builds (where the functional tests run) and never in Shipping. Tests reach the game only
 * through its exported API and the explicit test-access friends (FMvsInputTestAccess).
 */
public class MVVMSampleTests : ModuleRules
{
	public MVVMSampleTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"MVVMSample",
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore",
			"ModelViewViewModel", "FieldNotification",
			"CommonUI", "CommonInput",
			"GameplayTags", "DeveloperSettings"
		});
	}
}
