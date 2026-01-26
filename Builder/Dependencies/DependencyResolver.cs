using LumenBuilder.Common;

namespace LumenBuilder
{

    namespace Dependencies
    {

        /// <summary>
        /// External dependency info resolved from vendor or system.
        /// </summary>
        public readonly record struct ExternalDependency(
            string Name,
            IReadOnlyList<string> IncludePaths,
            IReadOnlyList<string> LibraryPaths,
            IReadOnlyList<string> Libraries
        );

        /// <summary>
        /// Resolves external dependencies from vendor directory or pkg-config.
        /// Thread-safe, pure IO operations with caching.
        /// </summary>
        public sealed class DependencyResolver
        {
            private readonly string VendorPath;
            private readonly PlatformInfo Platform;
            private readonly Dictionary<string, ExternalDependency?> Cache = new();
            private readonly object CacheLock = new();

            public DependencyResolver(string RootDirectory, PlatformInfo Platform)
            {
                this.VendorPath = Paths.Combine(RootDirectory, "Vendor");
                this.Platform = Platform;
            }

            /// <summary>
            /// Resolves a dependency by name. Returns null if not found.
            /// </summary>
            public ExternalDependency? Resolve(string Name)
            {
                lock (CacheLock)
                {
                    if (Cache.TryGetValue(Name, out var Cached))
                        return Cached;
                }

                var Result = TryResolveFromVendor(Name) ?? TryResolveFromPkgConfig(Name);

                lock (CacheLock)
                {
                    Cache[Name] = Result;
                }

                return Result;
            }

            /// <summary>
            /// Resolves multiple dependencies in parallel.
            /// </summary>
            public IReadOnlyDictionary<string, ExternalDependency> ResolveAll(IEnumerable<string> Names)
            {
                var Results = new Dictionary<string, ExternalDependency>();
                var ToResolve = new List<string>();

                lock (CacheLock)
                {
                    foreach (var Name in Names)
                    {
                        if (Cache.TryGetValue(Name, out var Cached) && Cached.HasValue)
                        {
                            Results[Name] = Cached.Value;
                        }
                        else
                        {
                            ToResolve.Add(Name);
                        }
                    }
                }

                var Resolved = new Dictionary<string, ExternalDependency>();
                var ResolvedLock = new object();

                Parallel.ForEach(ToResolve, Name =>
                {
                    var Dep = TryResolveFromVendor(Name) ?? TryResolveFromPkgConfig(Name);
                    if (Dep.HasValue)
                    {
                        lock (ResolvedLock)
                        {
                            Resolved[Name] = Dep.Value;
                        }
                    }
                });

                lock (CacheLock)
                {
                    foreach (var Kvp in Resolved)
                    {
                        Cache[Kvp.Key] = Kvp.Value;
                        Results[Kvp.Key] = Kvp.Value;
                    }
                }

                return Results;
            }

            private ExternalDependency? TryResolveFromVendor(string Name)
            {
                string DepPath = Paths.Combine(VendorPath, Name);

                if (!Directory.Exists(DepPath))
                    return null;

                var IncludePaths = new List<string>();
                var LibraryPaths = new List<string>();
                var Libraries = new List<string>();

                string IncludeDir = Paths.Combine(DepPath, "include");
                if (Directory.Exists(IncludeDir))
                {
                    IncludePaths.Add(IncludeDir);
                }

                string LibDir = Paths.Combine(DepPath, "lib");
                if (Directory.Exists(LibDir))
                {
                    LibraryPaths.Add(LibDir);
                    CollectLibraries(LibDir, Libraries);
                }

                string? PlatformLibDir = GetPlatformLibDir(DepPath);
                if (PlatformLibDir != null && Directory.Exists(PlatformLibDir))
                {
                    LibraryPaths.Add(PlatformLibDir);
                    CollectLibraries(PlatformLibDir, Libraries);
                }

                if (IncludePaths.Count == 0 && LibraryPaths.Count == 0)
                    return null;

                return new ExternalDependency(Name, IncludePaths, LibraryPaths, Libraries);
            }

            private ExternalDependency? TryResolveFromPkgConfig(string Name)
            {
                string PkgName = Name.ToLowerInvariant();

                if (Platform.Type == PlatformType.Windows)
                    return null;

                try
                {
                    var CFlags = RunPkgConfig(PkgName, "--cflags");
                    var Libs = RunPkgConfig(PkgName, "--libs");

                    if (CFlags == null && Libs == null)
                        return null;

                    var IncludePaths = new List<string>();
                    var LibraryPaths = new List<string>();
                    var Libraries = new List<string>();

                    if (CFlags != null)
                    {
                        ParseCFlags(CFlags, IncludePaths);
                    }

                    if (Libs != null)
                    {
                        ParseLibs(Libs, LibraryPaths, Libraries);
                    }

                    return new ExternalDependency(Name, IncludePaths, LibraryPaths, Libraries);
                }
                catch
                {
                    return null;
                }
            }

            private string? GetPlatformLibDir(string DepPath)
            {
                string? SubDir = Platform.Type switch
                {
                    PlatformType.Linux => "linux-x64",
                    PlatformType.Windows => Platform.Architecture == Architecture.Arm64 ? "windows-arm64" : "windows-x64",
                    PlatformType.MacOS => Platform.Architecture == Architecture.Arm64 ? "macos-arm64" : "macos-x64",
                    _ => null
                };

                return SubDir != null ? Paths.Combine(DepPath, SubDir) : null;
            }

            private void CollectLibraries(string LibDir, List<string> Libraries)
            {
                string Pattern = Platform.Type switch
                {
                    PlatformType.Windows => "*.lib",
                    _ => "*.so"
                };

                try
                {
                    string[] Files = Directory.GetFiles(LibDir, Pattern);
                    for (int FileIndex = 0; FileIndex < Files.Length; ++FileIndex)
                    {
                        string FileName = Path.GetFileNameWithoutExtension(Files[FileIndex]);

                        if (FileName.StartsWith("lib"))
                            FileName = FileName[3..];

                        if (!Libraries.Contains(FileName))
                            Libraries.Add(FileName);
                    }

                    string[] StaticFiles = Directory.GetFiles(LibDir, "*.a");
                    for (int FileIndex = 0; FileIndex < StaticFiles.Length; ++FileIndex)
                    {
                        string FileName = Path.GetFileNameWithoutExtension(StaticFiles[FileIndex]);

                        if (FileName.StartsWith("lib"))
                            FileName = FileName[3..];

                        if (!Libraries.Contains(FileName))
                            Libraries.Add(FileName);
                    }
                }
                catch { }
            }

            private string? RunPkgConfig(string Package, string Flag)
            {
                try
                {
                    var StartInfo = new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "pkg-config",
                        Arguments = $"{Flag} {Package}",
                        RedirectStandardOutput = true,
                        RedirectStandardError = true,
                        UseShellExecute = false,
                        CreateNoWindow = true
                    };

                    using var Proc = System.Diagnostics.Process.Start(StartInfo);
                    if (Proc == null)
                        return null;

                    string Output = Proc.StandardOutput.ReadToEnd().Trim();
                    Proc.WaitForExit(5000);

                    return Proc.ExitCode == 0 ? Output : null;
                }
                catch
                {
                    return null;
                }
            }

            private void ParseCFlags(string CFlags, List<string> IncludePaths)
            {
                string[] Parts = CFlags.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                for (int PartIndex = 0; PartIndex < Parts.Length; ++PartIndex)
                {
                    string Part = Parts[PartIndex];
                    if (Part.StartsWith("-I"))
                    {
                        string Path = Part[2..];
                        if (!IncludePaths.Contains(Path))
                            IncludePaths.Add(Path);
                    }
                }
            }

            private void ParseLibs(string Libs, List<string> LibraryPaths, List<string> Libraries)
            {
                string[] Parts = Libs.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                for (int PartIndex = 0; PartIndex < Parts.Length; ++PartIndex)
                {
                    string Part = Parts[PartIndex];
                    if (Part.StartsWith("-L"))
                    {
                        string Path = Part[2..];
                        if (!LibraryPaths.Contains(Path))
                            LibraryPaths.Add(Path);
                    }
                    else if (Part.StartsWith("-l"))
                    {
                        string Lib = Part[2..];
                        if (!Libraries.Contains(Lib))
                            Libraries.Add(Lib);
                    }
                }
            }
        }

    }

}
