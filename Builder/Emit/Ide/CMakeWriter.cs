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
                    Sb.AppendLine("set(CMAKE_CXX_STANDARD 23)");
                    Sb.AppendLine("set(CMAKE_CXX_STANDARD_REQUIRED ON)");
                    Sb.AppendLine("set(CMAKE_EXPORT_COMPILE_COMMANDS ON)");
                    Sb.AppendLine();

                    WriteBuildTypeConfig(Sb);
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
                /// Writes the module targets to the StringBuilder.
                /// </summary>
                private void WriteModuleTargets(StringBuilder Sb, BuildPlan Plan)
                {
                    for (int BuildOrdIndex = 0; BuildOrdIndex < Plan.BuildOrder.Count; ++BuildOrdIndex)
                    {
                        string ModuleName = Plan.BuildOrder[BuildOrdIndex];

                        if (!Context.Graph.TryGetModule(ModuleName, out var Module) || Module == null)
                        {
                            continue;
                        }

                        WriteModuleTarget(Sb, Module);
                        Sb.AppendLine();
                    }
                }

                /// <summary>
                /// Writes a single module target to the StringBuilder.
                /// </summary>
                private void WriteModuleTarget(StringBuilder Sb, ModuleDescriptor Module)
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

                    if (Module.PublicIncludes.Count > 0 || Module.PrivateIncludes.Count > 0)
                    {
                        Sb.Append("target_include_directories(");
                        Sb.Append(Module.Name);
                        Sb.AppendLine();

                        for (int PubIncIndex = 0; PubIncIndex < Module.PublicIncludes.Count; ++PubIncIndex)
                        {
                            Sb.Append("  PUBLIC ");
                            Sb.AppendLine(Paths.Combine(Module.Directory, Module.PublicIncludes[PubIncIndex]));
                        }

                        for (int PrivIncIndex = 0; PrivIncIndex < Module.PrivateIncludes.Count; ++PrivIncIndex)
                        {
                            Sb.Append("  PRIVATE ");
                            Sb.AppendLine(Paths.Combine(Module.Directory, Module.PrivateIncludes[PrivIncIndex]));
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

                    /** Add platform-specific system libraries */
                    var PlatformLibs = GetPlatformLibraries(Module);
                    if (PlatformLibs.Count > 0)
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
