// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BaseClasses : ModuleRules
{
	public BaseClasses(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core" 
			//,"AIModule", "GameplayTasks"		// AI Modules
            //,"RawInput", "InputDevice", "ApplicationCore"	// Lagt til angående ps4 gamepad support
		});	

		PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine", "InputCore", "Niagara", "GameplayTags" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
