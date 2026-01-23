using LumenBuilder.Common;
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

    public BuildPlanner(BuildContext Context)
    {
        this.Context = Context;
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

            var ObjectFiles = new List<string>();
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
        var SourceFiles = ExpandSources(Module);

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

        /** Collect dependencies from common deps */
        for (int DepIndex = 0; DepIndex < Module.Dependencies.Count; DepIndex++)
        {
            string DepName = Module.Dependencies[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectIncludesRecursive(DepModule, Includes, Visited, false);
            }
        }

        /** Collect dependencies from platform-specific deps */
        var PlatformDeps = GetPlatformDependencies(Module);
        for (int DepIndex = 0; DepIndex < PlatformDeps.Count; DepIndex++)
        {
            string DepName = PlatformDeps[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectIncludesRecursive(DepModule, Includes, Visited, false);
            }
        }
    }

    /// <summary>
    /// Expands source file patterns into concrete file paths with recursive search support.
    /// </summary>
    private List<string> ExpandSources(ModuleDescriptor Module)
    {
        var SourceFiles = new List<string>();

        for (int PatternIndex = 0; PatternIndex < Module.Sources.Count; PatternIndex++)
        {
            string Pattern = Module.Sources[PatternIndex];
            string FullPattern = Paths.Combine(Module.Directory, Pattern);

            /** Check for recursive pattern **/
            if (Pattern.Contains("**"))
            {
                string BaseDir = Module.Directory;
                string SearchPattern = Path.GetFileName(Pattern);
                
                /** Extract base directory from pattern before ** */
                int RecursiveIndex = Pattern.IndexOf("**");
                if (RecursiveIndex > 0)
                {
                    string BasePath = Pattern.Substring(0, RecursiveIndex).TrimEnd('/', '\\');
                    BaseDir = Paths.Combine(Module.Directory, BasePath);
                    
                    /** Extract search pattern after ** */
                    string Remaining = Pattern.Substring(RecursiveIndex + 2).TrimStart('/', '\\');
                    SearchPattern = Remaining;
                }

                if (Directory.Exists(BaseDir))
                {
                    string[] Files = Directory.GetFiles(BaseDir, SearchPattern, SearchOption.AllDirectories);
                    for (int FileIndex = 0; FileIndex < Files.Length; FileIndex++)
                    {
                        SourceFiles.Add(Paths.Normalize(Files[FileIndex]));
                    }
                }
            }
            /** Single directory pattern with * */
            else if (Pattern.Contains('*'))
            {
                string Dir = Paths.GetDirectory(FullPattern);
                string FilePattern = Path.GetFileName(FullPattern);

                if (Directory.Exists(Dir))
                {
                    string[] Files = Directory.GetFiles(Dir, FilePattern);
                    for (int FileIndex = 0; FileIndex < Files.Length; FileIndex++)
                    {
                        SourceFiles.Add(Paths.Normalize(Files[FileIndex]));
                    }
                }
            }
            /** Direct file path */
            else
            {
                SourceFiles.Add(FullPattern);
            }
        }

        return SourceFiles;
    }

    /// <summary>
    /// Gets platform-specific dependencies for current platform.
    /// </summary>
    private IReadOnlyList<string> GetPlatformDependencies(ModuleDescriptor Module)
    {
        return Context.Platform.Type switch
        {
            PlatformType.Linux => Module.PlatformDeps.Linux,
            PlatformType.Windows => Module.PlatformDeps.Windows,
            PlatformType.MacOS => Module.PlatformDeps.MacOS,
            _ => Array.Empty<string>()
        };
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
        /** Process common dependencies */
        for (int DepIndex = 0; DepIndex < Module.Dependencies.Count; DepIndex++)
        {
            string DepName = Module.Dependencies[DepIndex];
            if (!Visited.Add(DepName))
                continue;

            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                Result.Add(GetOutputFile(DepModule));
                CollectTransitiveDependenciesRecursive(DepModule, Result, Visited);
            }
        }

        /** Process platform-specific dependencies */
        var PlatformDeps = GetPlatformDependencies(Module);
        for (int DepIndex = 0; DepIndex < PlatformDeps.Count; DepIndex++)
        {
            string DepName = PlatformDeps[DepIndex];
            if (!Visited.Add(DepName))
                continue;

            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                Result.Add(GetOutputFile(DepModule));
                CollectTransitiveDependenciesRecursive(DepModule, Result, Visited);
            }
        }
    }

    /// <summary>
    /// Collects all transitive system libraries for a module.
    /// </summary>
    private List<string> CollectTransitiveSystemLibraries(ModuleDescriptor Module)
    {
        var Result = new List<string>();
        var Visited = new HashSet<string>();
        CollectTransitiveSystemLibrariesRecursive(Module, Result, Visited);
        return Result;
    }

    /// <summary>
    /// Recursively collects transitive system libraries.
    /// </summary>
    private void CollectTransitiveSystemLibrariesRecursive(
        ModuleDescriptor Module,
        List<string> Result,
        HashSet<string> Visited)
    {
        if (!Visited.Add(Module.Name))
            return;

        /** Add platform-specific system libraries */
        var PlatformLibs = GetPlatformLibraries(Module);
        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; LibIndex++)
        {
            string Lib = PlatformLibs[LibIndex];
            if (!Result.Contains(Lib))
            {
                Result.Add(Lib);
            }
        }

        /** Recursively collect from dependencies */
        for (int DepIndex = 0; DepIndex < Module.Dependencies.Count; DepIndex++)
        {
            string DepName = Module.Dependencies[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveSystemLibrariesRecursive(DepModule, Result, Visited);
            }
        }

        /** Recursively collect from platform-specific dependencies */
        var PlatformDeps = GetPlatformDependencies(Module);
        for (int DepIndex = 0; DepIndex < PlatformDeps.Count; DepIndex++)
        {
            string DepName = PlatformDeps[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveSystemLibrariesRecursive(DepModule, Result, Visited);
            }
        }
    }

    /// <summary>
    /// Collects all transitive library paths for a module.
    /// </summary>
    private List<string> CollectTransitiveLibraryPaths(ModuleDescriptor Module)
    {
        var Result = new List<string>();
        var Visited = new HashSet<string>();
        CollectTransitiveLibraryPathsRecursive(Module, Result, Visited);
        return Result;
    }

    /// <summary>
    /// Recursively collects transitive library paths.
    /// </summary>
    private void CollectTransitiveLibraryPathsRecursive(
        ModuleDescriptor Module,
        List<string> Result,
        HashSet<string> Visited)
    {
        if (!Visited.Add(Module.Name))
            return;

        /** LibraryPaths feature not yet implemented in ModuleDescriptor */

        /** Recursively collect from dependencies */
        for (int DepIndex = 0; DepIndex < Module.Dependencies.Count; DepIndex++)
        {
            string DepName = Module.Dependencies[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveLibraryPathsRecursive(DepModule, Result, Visited);
            }
        }

        /** Recursively collect from platform-specific dependencies */
        var PlatformDeps = GetPlatformDependencies(Module);
        for (int DepIndex = 0; DepIndex < PlatformDeps.Count; DepIndex++)
        {
            string DepName = PlatformDeps[DepIndex];
            if (Context.Graph.TryGetModule(DepName, out var DepModule) && DepModule != null)
            {
                CollectTransitiveLibraryPathsRecursive(DepModule, Result, Visited);
            }
        }
    }

    /// <summary>
    /// Gets platform-specific system libraries for current platform.
    /// </summary>
    private IReadOnlyList<string> GetPlatformLibraries(ModuleDescriptor Module)
    {
        return Context.Platform.Type switch
        {
            PlatformType.Linux => Module.PlatformLibs.Linux,
            PlatformType.Windows => Module.PlatformLibs.Windows,
            PlatformType.MacOS => Module.PlatformLibs.MacOS,
            _ => Array.Empty<string>()
        };
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