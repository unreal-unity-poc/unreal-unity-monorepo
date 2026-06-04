using System.IO;
using UnrealBuildTool;

public class RustEngine : ModuleRules
{
    public RustEngine(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore"
        });

        string repoRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../.."));
        PublicIncludePaths.Add(Path.Combine(repoRoot, "rust-engine", "include"));

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            string libraryPath = Path.Combine(
                ModuleDirectory,
                "../ThirdParty/RustEngineLibrary/Mac/librust_engine.dylib");

            PublicDefinitions.Add($"RUST_ENGINE_LIBRARY_PATH=TEXT(\"{libraryPath.Replace("\\", "/")}\")");
            RuntimeDependencies.Add(libraryPath);
        }
    }
}

