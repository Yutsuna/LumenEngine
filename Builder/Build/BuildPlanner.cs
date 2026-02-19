using LumenBuilder.Common;
using LumenBuilder.Dependencies;
using LumenBuilder.Graph;
using LumenBuilder.Model;

namespace LumenBuilder.Build;

/// <summary>
/// Represents a single compilation unit.
/// </summary>
public sealed record CompileUnit(
    string SourceFile,
    string ObjectFile,
    IReadOnlyList<string> Includes,
    IReadOnlyList<string> Defines
);

/// <summary>
/// Represents a link target.
/// </summary>
public sealed record LinkTarget(
    string OutputFile,
    ModuleType Type,
    IReadOnlyList<string> ObjectFiles,
    IReadOnlyList<string> DependencyNames,
    IReadOnlyList<string> DependencyPaths,
    IReadOnlyList<string> SystemLibraries,
    IReadOnlyList<string> LibraryPaths
);

/// <summary>
/// Build plan for a complete build.
/// </summary>
public sealed record BuildPlan(
    IReadOnlyList<CompileUnit> CompileUnits,
    IReadOnlyList<LinkTarget> LinkTargets,
    IReadOnlyList<string> BuildOrder
);

/// <summary>
/// Plans the build based on the module graph.
/// </summary>
public sealed class BuildPlanner
{
    private readonly BuildContext Context;
    private readonly PlatformType PlatformType;
    private readonly IReadOnlyDictionary<string, ExternalDependency> ResolvedExternalDeps;

    public BuildPlanner(BuildContext Context)
    {
        this.Context = Context;
        this.PlatformType = Context.Platform.Type;
        this.ResolvedExternalDeps = ResolveExternalDependencies();
    }

    private IReadOnlyDictionary<string, ExternalDependency> ResolveExternalDependencies()
    {
        var AllExternalLibs = new HashSet<string>();

        foreach (var Module in Context.Graph.Modules.Values)
        {
            var PlatformLibs = Module.PlatformLibs.ForPlatform(PlatformType);
            for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
            {
                AllExternalLibs.Add(PlatformLibs[LibIndex]);
            }
        }

        var Requested = AllExternalLibs.Count > 0
            ? string.Join(", ", AllExternalLibs)
            : "none";
        Context.Diagnostics.Info($"External dependencies requested: {Requested}");

        var Resolved = Context.ExternalDeps.ResolveAll(AllExternalLibs);
        var ResolvedKeys = Resolved.Count > 0
            ? string.Join(", ", Resolved.Keys)
            : "none";
        Context.Diagnostics.Info($"Resolved external dependencies: {ResolvedKeys}");

        return Resolved;
    }

    public BuildPlan CreatePlan()
    {
        var DepGraph = new DependencyGraph();
        foreach (var Module in Context.Graph.Modules.Values)
        {
            DepGraph.AddModule(Module);
        }

        var Detector = new CycleDetector(DepGraph);
        if (Detector.HasCycle(out var Cycle))
        {
            Context.Diagnostics.Error($"Dependency cycle detected: {string.Join(" -> ", Cycle)}");
            return new BuildPlan(
                Array.Empty<CompileUnit>(),
                Array.Empty<LinkTarget>(),
                Array.Empty<string>()
            );
        }

        var Sorter = new TopologicalSorter(DepGraph);
        var BuildOrder = Sorter.Sort();

        var CompileUnits = new List<CompileUnit>();
        var LinkTargets = new List<LinkTarget>();

        for (int ModuleIndex = 0; ModuleIndex < BuildOrder.Count; ModuleIndex++)
        {
            string ModuleName = BuildOrder[ModuleIndex];
            if (!Context.Graph.TryGetModule(ModuleName, out var Module) || Module == null)
            {
                Context.Diagnostics.Warning($"Module not found: {ModuleName}");
                continue;
            }

            var ModuleCompileUnits = CreateCompileUnits(Module);
            CompileUnits.AddRange(ModuleCompileUnits);

            var ObjectFiles = new List<string>(ModuleCompileUnits.Count);
            for (int UnitIndex = 0; UnitIndex < ModuleCompileUnits.Count; UnitIndex++)
            {
                ObjectFiles.Add(ModuleCompileUnits[UnitIndex].ObjectFile);
            }

            string OutputFile = GetOutputFile(Module);
            var DependencyPaths = CollectTransitiveDependencies(Module);
            var SystemLibs = CollectTransitiveSystemLibraries(Module);
            var LibraryPaths = CollectTransitiveLibraryPaths(Module);

            LinkTargets.Add(new LinkTarget(
                OutputFile,
                Module.Type,
                ObjectFiles,
                Module.Dependencies,
                DependencyPaths,
                SystemLibs,
                LibraryPaths
            ));
        }

        return new BuildPlan(CompileUnits, LinkTargets, BuildOrder);
    }

    private List<CompileUnit> CreateCompileUnits(ModuleDescriptor Module)
    {
        var Units = new List<CompileUnit>();
        var Includes = CollectIncludes(Module);
        var SourceFiles = SourceExpander.Expand(Module);

        for (int FileIndex = 0; FileIndex < SourceFiles.Count; FileIndex++)
        {
            string SourceFile = SourceFiles[FileIndex];
            string ObjectFile = GetObjectFile(Module, SourceFile);
            Units.Add(new CompileUnit(SourceFile, ObjectFile, Includes, Module.Defines));
        }

        return Units;
    }

    private List<string> CollectIncludes(ModuleDescriptor Module)
    {
        var Includes = new List<string>();
        var Visited = new HashSet<string>();

        CollectIncludesRecursive(Module, Includes, Visited, true);
        AppendExternalIncludes(Module, Includes);

        return Includes;
    }

    private void CollectIncludesRecursive(
        ModuleDescriptor Module,
        List<string> Includes,
        HashSet<string> Visited,
        bool IncludePrivate)
    {
        if (!Visited.Add(Module.Name))
            return;

        if (IncludePrivate)
        {
            for (int IncludeIndex = 0; IncludeIndex < Module.PrivateIncludes.Count; IncludeIndex++)
            {
                Includes.Add(Paths.Combine(Module.Directory, Module.PrivateIncludes[IncludeIndex]));
            }
        }

        for (int IncludeIndex = 0; IncludeIndex < Module.PublicIncludes.Count; IncludeIndex++)
        {
            Includes.Add(Paths.Combine(Module.Directory, Module.PublicIncludes[IncludeIndex]));
        }

        AppendExternalIncludes(Module, Includes);

        foreach (var DepName in EnumerateAllDependencies(Module))
        {
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectIncludesRecursive(DepModule, Includes, Visited, false);
            }
        }
    }

    private void AppendExternalIncludes(ModuleDescriptor Module, List<string> Includes)
    {
        var PlatformLibs = Module.PlatformLibs.ForPlatform(PlatformType);
        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
        {
            if (ResolvedExternalDeps.TryGetValue(PlatformLibs[LibIndex], out var ExtDep))
            {
                for (int IncIndex = 0; IncIndex < ExtDep.IncludePaths.Count; ++IncIndex)
                {
                    if (!Includes.Contains(ExtDep.IncludePaths[IncIndex]))
                    {
                        Includes.Add(ExtDep.IncludePaths[IncIndex]);
                    }
                }
            }
        }
    }

    /// <summary>
    /// Enumerates common + platform-specific dependencies for a module.
    /// </summary>
    private IEnumerable<string> EnumerateAllDependencies(ModuleDescriptor Module)
    {
        for (int Index = 0; Index < Module.Dependencies.Count; ++Index)
            yield return Module.Dependencies[Index];

        var PlatformDeps = Module.PlatformDeps.ForPlatform(PlatformType);
        for (int Index = 0; Index < PlatformDeps.Count; ++Index)
            yield return PlatformDeps[Index];
    }

    private List<string> CollectTransitiveDependencies(ModuleDescriptor Module)
    {
        var Result = new List<string>();
        var Visited = new HashSet<string>();
        CollectTransitiveDependenciesRecursive(Module, Result, Visited);
        return Result;
    }

    private void CollectTransitiveDependenciesRecursive(
        ModuleDescriptor Module,
        List<string> Result,
        HashSet<string> Visited)
    {
        foreach (var DepName in EnumerateAllDependencies(Module))
        {
            if (!Visited.Add(DepName))
                continue;

            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                Result.Add(GetOutputFile(DepModule));
                CollectTransitiveDependenciesRecursive(DepModule, Result, Visited);
            }
        }
    }

    private List<string> CollectTransitiveSystemLibraries(ModuleDescriptor Module)
    {
        var Result = new List<string>();
        var Visited = new HashSet<string>();
        CollectTransitiveSystemLibrariesRecursive(Module, Result, Visited);
        return Result;
    }

    private void CollectTransitiveSystemLibrariesRecursive(
        ModuleDescriptor Module,
        List<string> Result,
        HashSet<string> Visited)
    {
        if (!Visited.Add(Module.Name))
            return;

        var PlatformLibs = Module.PlatformLibs.ForPlatform(PlatformType);
        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
        {
            if (!Result.Contains(PlatformLibs[LibIndex]))
            {
                Result.Add(PlatformLibs[LibIndex]);
            }
        }

        foreach (var DepName in EnumerateAllDependencies(Module))
        {
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveSystemLibrariesRecursive(DepModule, Result, Visited);
            }
        }
    }

    private List<string> CollectTransitiveLibraryPaths(ModuleDescriptor Module)
    {
        var Result = new List<string>();
        var Visited = new HashSet<string>();
        CollectTransitiveLibraryPathsRecursive(Module, Result, Visited);
        AppendExternalLibraryPaths(Module, Result);
        return Result;
    }

    private void CollectTransitiveLibraryPathsRecursive(
        ModuleDescriptor Module,
        List<string> Result,
        HashSet<string> Visited)
    {
        if (!Visited.Add(Module.Name))
            return;

        AppendExternalLibraryPaths(Module, Result);

        foreach (var DepName in EnumerateAllDependencies(Module))
        {
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveLibraryPathsRecursive(DepModule, Result, Visited);
            }
        }
    }

    private void AppendExternalLibraryPaths(ModuleDescriptor Module, List<string> Result)
    {
        var PlatformLibs = Module.PlatformLibs.ForPlatform(PlatformType);
        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
        {
            if (ResolvedExternalDeps.TryGetValue(PlatformLibs[LibIndex], out var ExtDep))
            {
                for (int PathIndex = 0; PathIndex < ExtDep.LibraryPaths.Count; ++PathIndex)
                {
                    if (!Result.Contains(ExtDep.LibraryPaths[PathIndex]))
                    {
                        Result.Add(ExtDep.LibraryPaths[PathIndex]);
                    }
                }
            }
        }
    }

    private string GetObjectFile(ModuleDescriptor Module, string SourceFile)
    {
        string FileName = Path.GetFileNameWithoutExtension(SourceFile);
        string IntermediatePath = Paths.GetIntermediatePath(Context.OutputDirectory, Module.Name);
        return Paths.Combine(IntermediatePath, FileName + Context.Platform.ObjectExtension);
    }

    private string GetOutputFile(ModuleDescriptor Module)
    {
        string BinaryPath = Paths.GetBinaryPath(Context.OutputDirectory, Module.Name);
        string Extension = Module.Type switch
        {
            ModuleType.Executable => Context.Platform.ExecutableExtension,
            ModuleType.SharedLibrary => Context.Platform.SharedLibExtension,
            _ => Context.Platform.StaticLibExtension
        };
        return Paths.Combine(BinaryPath, Module.Name + Extension);
    }
}