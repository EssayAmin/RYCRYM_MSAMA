// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyProject3 : ModuleRules
{
	public MyProject3(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" , "AIModule", "NavigationSystem", "GameplayTasks" });

        PrivateDependencyModuleNames.AddRange(new string[] { });

        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
    }
}
