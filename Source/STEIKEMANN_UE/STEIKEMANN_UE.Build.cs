// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class STEIKEMANN_UE : ModuleRules
{
	public STEIKEMANN_UE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core"  /*, "Niagara" *//*, "GameplayTags"*/
			,"AIModule", "GameplayTasks"		// AI Modules
			//,"BaseClasses"
            //,"RawInput", "InputDevice", "ApplicationCore"	// Lagt til angående ps4 gamepad support
		});	

		PrivateDependencyModuleNames.AddRange(new string[] { "BaseClasses", "MechanicTestingClasses", "Niagara", "GameplayTags", "CoreUObject", "Engine", "InputCore" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
