using System.Text;

using LumenBuilder.Build;
using LumenBuilder.Common;
using LumenBuilder.Model;

namespace LumenBuilder
{

    namespace Emit
    {

        namespace Ide
        {

            /// <summary>
            /// Emits CMakeLists.txt for IDE integration.
            /// </summary>
            public sealed class CMakeWriter
            {
                private readonly BuildContext Context;

                /// <summary>
                /// Creates a new CMake writer with the given build context.
                /// </summary>
                public CMakeWriter(BuildContext Context)
                {
                    this.Context = Context;
                }

                /// <summary>
                /// Generates the CMakeLists.txt content for the given build plan.
                /// </summary>
                public string Generate(BuildPlan Plan)
                {
                    var Sb = new StringBuilder();

                    Sb.AppendLine("cmake_minimum_required(VERSION 3.20)");
                    Sb.AppendLine($"project(LumenEngine CXX)");
                    Sb.AppendLine();
                    
                    WriteToolchainConfig(Sb);
                    Sb.AppendLine();
                    
                    Sb.AppendLine("set(CMAKE_CXX_STANDARD 23)");
                    Sb.AppendLine("set(CMAKE_CXX_STANDARD_REQUIRED ON)");
                    Sb.AppendLine("set(CMAKE_EXPORT_COMPILE_COMMANDS ON)");
                    Sb.AppendLine();

                    WriteBuildTypeConfig(Sb);
                    Sb.AppendLine();
                    
                    WritePlatformConfig(Sb);
                    Sb.AppendLine();

                    WriteModuleTargets(Sb, Plan);

                    return Sb.ToString();
                }

                /// <summary>
                /// Writes the CMakeLists.txt file to the specified output path.
                /// </summary>
                public void Write(string OutputPath, BuildPlan Plan)
                {
                    string Content = Generate(Plan);
                    string Dir = Path.GetDirectoryName(OutputPath) ?? ".";

                    if (!Directory.Exists(Dir))
                    {
                        Directory.CreateDirectory(Dir);
                    }
                    File.WriteAllText(OutputPath, Content);
                }

                /// <summary>
                /// Writes the toolchain configuration to the StringBuilder.
                /// </summary>
                private void WriteToolchainConfig(StringBuilder Sb)
                {
                    bool IsMsvc = Context.Toolchain.Name == "msvc";
                    bool IsWindows = Context.Platform.Type == PlatformType.Windows;
                    
                    if (IsWindows && IsMsvc)
                    {
                        Sb.AppendLine("# MSVC Toolchain Configuration");
                        Sb.AppendLine("set(CMAKE_CXX_COMPILER \"cl.exe\")");
                        Sb.AppendLine("set(CMAKE_C_COMPILER \"cl.exe\")");
                    }
                    else if (IsWindows && !IsMsvc)
                    {
                        Sb.AppendLine("# Clang/GCC Toolchain on Windows");
                        Sb.AppendLine($"set(CMAKE_CXX_COMPILER \"{Context.Toolchain.CompilerPath}\")");
                    }
                }
                
                /// <summary>
                /// Writes the build type configuration to the StringBuilder.
                /// </summary>
                private void WriteBuildTypeConfig(StringBuilder Sb)
                {
                    Sb.AppendLine("if(NOT CMAKE_BUILD_TYPE)");
                    string DefaultType = Context.Configuration switch
                    {
                        BuildConfiguration.Debug => "Debug",
                        BuildConfiguration.Release => "Release",
                        _ => "RelWithDebInfo"
                    };
                    Sb.AppendLine($"  set(CMAKE_BUILD_TYPE {DefaultType})");
                    Sb.AppendLine("endif()");
                }
                
                /// <summary>
                /// Writes the platform-specific configuration to the StringBuilder.
                /// </summary>
                private void WritePlatformConfig(StringBuilder Sb)
                {
                    bool IsMsvc = Context.Toolchain.Name == "msvc";
                    bool IsWindows = Context.Platform.Type == PlatformType.Windows;
                    
                    if (IsWindows)
                    {
                        Sb.AppendLine("# Windows Platform Configuration");
                        if (IsMsvc)
                        {
                            Sb.AppendLine("add_compile_options(/EHsc /W3)");
                            Sb.AppendLine("add_definitions(-DWIN32 -D_WINDOWS)");
                        }
                        else
                        {
                            Sb.AppendLine("add_definitions(-DWIN32 -D_WINDOWS)");
                        }
                    }
                    else if (Context.Platform.Type == PlatformType.Linux)
                    {
                        Sb.AppendLine("# Linux Platform Configuration");
                        Sb.AppendLine("add_definitions(-DLINUX)");
                    }
                    else if (Context.Platform.Type == PlatformType.MacOS)
                    {
                        Sb.AppendLine("# MacOS Platform Configuration");
                        Sb.AppendLine("add_definitions(-DMACOS)");
                    }
                }

                /// <summary>
                /// Writes the module targets to the StringBuilder.
                /// </summary>
                private void WriteModuleTargets(StringBuilder Sb, BuildPlan Plan)
                {
                    var LinkTargetLookup = new Dictionary<string, LinkTarget>();

                    for (int i = 0; i < Plan.LinkTargets.Count && i < Plan.BuildOrder.Count; ++i)
                    {
                        var Target = Plan.LinkTargets[i];
                        string ModuleName = Plan.BuildOrder[i];
                        LinkTargetLookup[ModuleName] = Target;
                    }

                    for (int BuildOrdIndex = 0; BuildOrdIndex < Plan.BuildOrder.Count; ++BuildOrdIndex)
                    {
                        string ModuleName = Plan.BuildOrder[BuildOrdIndex];

                        if (!Context.Graph.TryGetModule(ModuleName, out var Module) || Module == null)
                        {
                            continue;
                        }

                        LinkTargetLookup.TryGetValue(ModuleName, out var LinkTarget);
                        WriteModuleTarget(Sb, Module, LinkTarget);
                        Sb.AppendLine();
                    }
                }

                /// <summary>
                /// Writes a single module target to the StringBuilder.
                /// </summary>
                private void WriteModuleTarget(StringBuilder Sb, ModuleDescriptor Module, LinkTarget? LinkTarget)
                {
                    var SourceFiles = GetSourceFiles(Module);

                    string TargetType = Module.Type switch
                    {
                        ModuleType.Executable => "add_executable",
                        ModuleType.SharedLibrary => "add_library",
                        _ => "add_library"
                    };

                    Sb.Append(TargetType);
                    Sb.Append('(');
                    Sb.Append(Module.Name);

                    if (Module.Type == ModuleType.StaticLibrary)
                    {
                        Sb.Append(" STATIC");
                    }
                    else if (Module.Type == ModuleType.SharedLibrary)
                    {
                        Sb.Append(" SHARED");
                    }

                    Sb.AppendLine();

                    for (int SrcIndex = 0; SrcIndex < SourceFiles.Count; ++SrcIndex)
                    {
                        Sb.Append("  ");
                        Sb.AppendLine(SourceFiles[SrcIndex]);
                    }
                    Sb.AppendLine(")");

                    // Set output directory to match Ninja/Makefile output structure
                    string OutputDir = Paths.GetBinaryPath(Context.OutputDirectory, Module.Name);
                    if (Module.Type == ModuleType.Executable)
                    {
                        Sb.Append("set_target_properties(");
                        Sb.Append(Module.Name);
                        Sb.Append(" PROPERTIES RUNTIME_OUTPUT_DIRECTORY ");
                        Sb.Append(OutputDir);
                        Sb.AppendLine(")");
                    }
                    else
                    {
                        Sb.Append("set_target_properties(");
                        Sb.Append(Module.Name);
                        Sb.Append(" PROPERTIES LIBRARY_OUTPUT_DIRECTORY ");
                        Sb.Append(OutputDir);
                        Sb.Append(" ARCHIVE_OUTPUT_DIRECTORY ");
                        Sb.Append(OutputDir);
                        Sb.AppendLine(")");
                    }

                    // Collect all include directories: module includes + external dependency includes
                    var AllIncludes = new List<string>();
                    for (int PubIncIndex = 0; PubIncIndex < Module.PublicIncludes.Count; ++PubIncIndex)
                    {
                        AllIncludes.Add(Paths.Combine(Module.Directory, Module.PublicIncludes[PubIncIndex]));
                    }
                    for (int PrivIncIndex = 0; PrivIncIndex < Module.PrivateIncludes.Count; ++PrivIncIndex)
                    {
                        AllIncludes.Add(Paths.Combine(Module.Directory, Module.PrivateIncludes[PrivIncIndex]));
                    }

                    // Add external dependency includes from LinkTarget (e.g., SDL3)
                    var PlatformLibs = GetPlatformLibraries(Module);
                    for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
                    {
                        string LibName = PlatformLibs[LibIndex];
                        var ExtDep = Context.ExternalDeps.Resolve(LibName);
                        if (ExtDep.HasValue)
                        {
                            for (int IncIndex = 0; IncIndex < ExtDep.Value.IncludePaths.Count; ++IncIndex)
                            {
                                if (!AllIncludes.Contains(ExtDep.Value.IncludePaths[IncIndex]))
                                {
                                    AllIncludes.Add(ExtDep.Value.IncludePaths[IncIndex]);
                                }
                            }
                        }
                    }

                    if (AllIncludes.Count > 0)
                    {
                        Sb.Append("target_include_directories(");
                        Sb.Append(Module.Name);
                        Sb.AppendLine(" PUBLIC");

                        for (int IncIndex = 0; IncIndex < AllIncludes.Count; ++IncIndex)
                        {
                            Sb.Append("  ");
                            Sb.AppendLine(AllIncludes[IncIndex]);
                        }
                        Sb.AppendLine(")");
                    }

                    if (Module.Defines.Count > 0)
                    {
                        Sb.Append("target_compile_definitions(");
                        Sb.Append(Module.Name);
                        Sb.Append(" PRIVATE");

                        for (int DefIndex = 0; DefIndex < Module.Defines.Count; ++DefIndex)
                        {
                            Sb.Append(' ');
                            Sb.Append(Module.Defines[DefIndex]);
                        }
                        Sb.AppendLine(")");
                    }

                    if (Module.Dependencies.Count > 0)
                    {
                        Sb.Append("target_link_libraries(");
                        Sb.Append(Module.Name);
                        Sb.Append(" PUBLIC");

                        for (int DepsIndex = 0; DepsIndex < Module.Dependencies.Count; ++DepsIndex)
                        {
                            Sb.Append(' ');
                            Sb.Append(Module.Dependencies[DepsIndex]);
                        }
                        Sb.AppendLine(")");
                    }

                    /** Add library paths and system libraries from LinkTarget */
                    if (LinkTarget != null && (LinkTarget.LibraryPaths.Count > 0 || LinkTarget.SystemLibraries.Count > 0))
                    {
                        if (LinkTarget.LibraryPaths.Count > 0)
                        {
                            Sb.Append("target_link_directories(");
                            Sb.Append(Module.Name);
                            Sb.AppendLine(" PUBLIC");
                            for (int LibPathIndex = 0; LibPathIndex < LinkTarget.LibraryPaths.Count; ++LibPathIndex)
                            {
                                Sb.Append("  ");
                                Sb.AppendLine(LinkTarget.LibraryPaths[LibPathIndex]);
                            }
                            Sb.AppendLine(")");
                        }

                        if (LinkTarget.SystemLibraries.Count > 0)
                        {
                            Sb.Append("target_link_libraries(");
                            Sb.Append(Module.Name);
                            Sb.Append(" PUBLIC");

                            for (int SysLibIndex = 0; SysLibIndex < LinkTarget.SystemLibraries.Count; ++SysLibIndex)
                            {
                                Sb.Append(' ');
                                Sb.Append(LinkTarget.SystemLibraries[SysLibIndex]);
                            }
                            Sb.AppendLine(")");
                        }
                    }
                    else if (PlatformLibs.Count > 0)
                    {
                        Sb.Append("target_link_libraries(");
                        Sb.Append(Module.Name);
                        Sb.Append(" PUBLIC");

                        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
                        {
                            Sb.Append(' ');
                            Sb.Append(PlatformLibs[LibIndex]);
                        }
                        Sb.AppendLine(")");
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

                /// <summary>
                /// Gets the list of source files for the given module, expanding any wildcards.
                /// </summary>
                private List<string> GetSourceFiles(ModuleDescriptor Module)
                {
                    var SourceFiles = new List<string>();

                    for (int PatternIndex = 0; PatternIndex < Module.Sources.Count; ++PatternIndex)
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
                                for (int FileIndex = 0; FileIndex < Files.Length; ++FileIndex)
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
                                for (int FileIndex = 0; FileIndex < Files.Length; ++FileIndex)
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

            }

        }

    }

}
