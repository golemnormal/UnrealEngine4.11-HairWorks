// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WmfMedia : ModuleRules
	{
		public WmfMedia(TargetInfo Target)
		{
            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                    "Media",
				}
            );

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
                    "RenderCore",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
                    "Media",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"WmfMedia/Private",
                    "WmfMedia/Private/Player",
                    "WmfMedia/Private/Tracks",
                    "WmfMedia/Private/Wmf",
				}
			);

            if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
            {
                PublicDelayLoadDLLs.Add("shlwapi.dll");
                PublicDelayLoadDLLs.Add("mf.dll");
                PublicDelayLoadDLLs.Add("mfplat.dll");
                PublicDelayLoadDLLs.Add("mfplay.dll");
                PublicDelayLoadDLLs.Add("mfuuid.dll");
            }
		}
	}
}
