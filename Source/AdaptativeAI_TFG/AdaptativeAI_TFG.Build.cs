// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AdaptativeAI_TFG : ModuleRules
{
	public AdaptativeAI_TFG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"AdaptativeAI_TFG",
			"AdaptativeAI_TFG/Variant_Horror",
			"AdaptativeAI_TFG/Variant_Horror/UI",
			"AdaptativeAI_TFG/Variant_Shooter",
			"AdaptativeAI_TFG/Variant_Shooter/AI",
			"AdaptativeAI_TFG/Variant_Shooter/UI",
			"AdaptativeAI_TFG/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
