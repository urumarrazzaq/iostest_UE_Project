// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using EpicGames.Core;

public class HealthKitPlugin : ModuleRules
{
	public HealthKitPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicDefinitions.Add("__OBJC__=1");
			//bEnableObjCExceptions = true;


			string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty/iOS");

			// Add include path for the headers
			PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "include"));

			// Add the static library (.a file)
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "libHealthKitStaticLib.a"));

			// Add required frameworks for HealthKit
			PublicFrameworks.AddRange(
				new string[]
				{
					"HealthKit",
					"Foundation",
					"CoreMotion"
				}
			);

			AdditionalBundleResources.Add(new BundleResource("HealthKit.entitlements"));
		}

		PublicFrameworks.AddRange(new string[]
            {
                "HealthKit",
                "Foundation",
                "CoreMotion"
            });
            
      
		
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
