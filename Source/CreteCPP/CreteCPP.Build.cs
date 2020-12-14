// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CreteCPP : ModuleRules
{
	public CreteCPP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "GameLiftServerSDK", "UMG", "SlateCore", "Http", "Json", "JsonUtilities", "WebBrowserWidget" });
	}
}
