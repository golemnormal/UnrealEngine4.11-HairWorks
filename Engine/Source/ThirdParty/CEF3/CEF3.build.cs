// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CEF3 : ModuleRules
{
	public CEF3(TargetInfo Target)
	{
		/** Mark the current version of the library */
		string CEFVersion = "3.2272.2077";
		string CEFPlatform = "";

		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			CEFPlatform = "windows64";
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			CEFPlatform = "windows32";
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			CEFVersion = "3.2272.24.ga8ab9ad";
			CEFPlatform = "macosx64";
		}

		if (CEFPlatform.Length > 0 && UEBuildConfiguration.bCompileCEF3)
		{
			Definitions.Add("WITH_CEF3=1");

			string PlatformPath = Path.Combine(UEBuildConfiguration.UEThirdPartySourceDirectory, "CEF3", "cef_binary_" + CEFVersion + "_" + CEFPlatform);

			PublicSystemIncludePaths.Add(PlatformPath);

			string LibraryPath = Path.Combine(PlatformPath, "Release");
            string RuntimePath = Path.Combine(UEBuildConfiguration.UEThirdPartyBinariesDirectory , "CEF3", Target.Platform.ToString());

			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
			{
                string VSVersionFolderName = "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
                LibraryPath = Path.Combine(LibraryPath, VSVersionFolderName);

                PublicLibraryPaths.Add(LibraryPath);

				PublicAdditionalLibraries.Add("libcef.lib");
				if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
				{
					PublicAdditionalLibraries.Add("libcef_dll_wrapperD.lib");
				}
				else
				{
					PublicAdditionalLibraries.Add("libcef_dll_wrapper.lib");
				}

                
                string[] Dlls = {
                    "d3dcompiler_43.dll",
                    "d3dcompiler_47.dll",
                    "ffmpegsumo.dll",
                    "libcef.dll",
                    "libEGL.dll",
                    "libGLESv2.dll",
                    "pdf.dll",
                };

				PublicDelayLoadDLLs.AddRange(Dlls);

                // Add the runtime dlls to the build receipt
                foreach (string Dll in Dlls)
                {
                    RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/" + Dll));
                }
                // We also need the icu translations table required by CEF
                RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/ThirdParty/CEF3/" + Target.Platform.ToString() + "/icudtl.dat"));

                // And the entire Resources folder. Enunerate the entire directory instead of mentioning each file manually here.
                foreach (string FileName in Directory.EnumerateFiles(Path.Combine(RuntimePath, "Resources"), "*", SearchOption.AllDirectories))
                {
                    string DependencyName = FileName.Substring(UEBuildConfiguration.UEThirdPartyBinariesDirectory.Length).Replace('\\', '/');
                    RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/ThirdParty/" + DependencyName));
                }

                // UnrealCefSubprocess should also be added @Tdodo is this the right place to mention this? Seems like a reversal of dependencies to me
                RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/" + Target.Platform.ToString() + "/UnrealCEFSubProcess.exe"));
            }
			// TODO: Ensure these are filled out correctly when adding other platforms
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				string WrapperPath = LibraryPath + "/libcef_dll_wrapper.a";
                string FrameworkPath = UEBuildConfiguration.UEThirdPartyBinariesDirectory + "CEF3/Mac/Chromium Embedded Framework.framework";

				PublicAdditionalLibraries.Add(WrapperPath);
                PublicFrameworks.Add(FrameworkPath);

                var LocaleFolders = Directory.GetFileSystemEntries(LibraryPath + "/locale", "*.lproj");
				foreach (var FolderName in LocaleFolders)
				{
					AdditionalBundleResources.Add(new UEBuildBundleResource(FolderName, bInShouldLog:false));
				}

				// Add contents of framework directory as runtime dependencies
				foreach (string FilePath in Directory.EnumerateFiles(FrameworkPath, "*", SearchOption.AllDirectories))
				{
					RuntimeDependencies.Add(new RuntimeDependency(FilePath));
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				if (Target.IsMonolithic)
				{
					PublicAdditionalLibraries.Add(LibraryPath + "/libcef.a");
				}
				else
				{
					PublicLibraryPaths.Add(LibraryPath);
					PublicAdditionalLibraries.Add("libcef");
				}
			}
		}
	}
}
