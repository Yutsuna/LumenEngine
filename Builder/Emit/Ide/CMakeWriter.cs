using System.Text;

using LumenBuilder.Build;
using LumenBuilder.Common;
using LumenBuilder.Model;

namespace LumenBuilder.Emit.Ide;

/// <summary>
/// Emits CMakeLists.txt for IDE integration.
/// </summary>
public sealed class CMakeWriter
{
    private readonly BuildContext Context;
    private readonly PlatformType PlatformType;

    public CMakeWriter(BuildContext Context)
    {
        this.Context = Context;
        this.PlatformType = Context.Platform.Type;
    }

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

    public void Write(string OutputPath, BuildPlan Plan)
    {
        EmitHelpers.WriteToFile(OutputPath, Generate(Plan));
    }

    private void WriteToolchainConfig(StringBuilder Sb)
    {
        if (PlatformType == PlatformType.Windows)
        {
            Sb.AppendLine("# Clang/GCC Toolchain on Windows");
            Sb.AppendLine($"set(CMAKE_CXX_COMPILER \"{Context.Toolchain.CompilerPath}\")");
        }

        string LinkerFlag = Context.Toolchain.LinkerFlag;
        if (!string.IsNullOrEmpty(LinkerFlag))
        {
            Sb.AppendLine($"# Faster linker detected: {LinkerFlag}");
            Sb.AppendLine($"add_link_options({LinkerFlag})");
        }
    }

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

    private void WritePlatformConfig(StringBuilder Sb)
    {
        switch (PlatformType)
        {
            case PlatformType.Windows:
                Sb.AppendLine("# Windows Platform Configuration");
                Sb.AppendLine("add_definitions(-DWIN32 -D_WINDOWS)");
                break;
            case PlatformType.Linux:
                Sb.AppendLine("# Linux Platform Configuration");
                Sb.AppendLine("add_definitions(-DLINUX)");
                break;
            case PlatformType.MacOS:
                Sb.AppendLine("# MacOS Platform Configuration");
                Sb.AppendLine("add_definitions(-DMACOS)");
                break;
        }
    }

    private void WriteModuleTargets(StringBuilder Sb, BuildPlan Plan)
    {
        var LinkTargetLookup = new Dictionary<string, LinkTarget>();

        for (int Index = 0; Index < Plan.LinkTargets.Count && Index < Plan.BuildOrder.Count; ++Index)
        {
            LinkTargetLookup[Plan.BuildOrder[Index]] = Plan.LinkTargets[Index];
        }

        for (int Index = 0; Index < Plan.BuildOrder.Count; ++Index)
        {
            string ModuleName = Plan.BuildOrder[Index];

            if (!Context.Graph.TryGetModule(ModuleName, out var Module) || Module == null)
                continue;

            LinkTargetLookup.TryGetValue(ModuleName, out var LinkTarget);
            WriteModuleTarget(Sb, Module, LinkTarget);
            Sb.AppendLine();
        }
    }

    private void WriteModuleTarget(StringBuilder Sb, ModuleDescriptor Module, LinkTarget? LinkTarget)
    {
        var SourceFiles = SourceExpander.Expand(Module);

        string TargetType = Module.Type == ModuleType.Executable ? "add_executable" : "add_library";

        Sb.Append(TargetType);
        Sb.Append('(');
        Sb.Append(Module.Name);

        if (Module.Type == ModuleType.StaticLibrary)
            Sb.Append(" STATIC");
        else if (Module.Type == ModuleType.SharedLibrary)
            Sb.Append(" SHARED");

        Sb.AppendLine();

        for (int Index = 0; Index < SourceFiles.Count; ++Index)
        {
            Sb.Append("  ");
            Sb.AppendLine(SourceFiles[Index]);
        }
        Sb.AppendLine(")");

        string OutputDir = Paths.GetBinaryPath(Context.OutputDirectory, Module.Name);
        if (Module.Type == ModuleType.Executable)
        {
            Sb.AppendLine($"set_target_properties({Module.Name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY {OutputDir})");
        }
        else
        {
            Sb.AppendLine($"set_target_properties({Module.Name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY {OutputDir} ARCHIVE_OUTPUT_DIRECTORY {OutputDir})");
        }

        var AllIncludes = new List<string>();
        for (int Index = 0; Index < Module.PublicIncludes.Count; ++Index)
            AllIncludes.Add(Paths.Combine(Module.Directory, Module.PublicIncludes[Index]));
        for (int Index = 0; Index < Module.PrivateIncludes.Count; ++Index)
            AllIncludes.Add(Paths.Combine(Module.Directory, Module.PrivateIncludes[Index]));

        var PlatformLibs = Module.PlatformLibs.ForPlatform(PlatformType);
        for (int LibIndex = 0; LibIndex < PlatformLibs.Count; ++LibIndex)
        {
            var ExtDep = Context.ExternalDeps.Resolve(PlatformLibs[LibIndex]);
            if (ExtDep.HasValue)
            {
                for (int IncIndex = 0; IncIndex < ExtDep.Value.IncludePaths.Count; ++IncIndex)
                {
                    if (!AllIncludes.Contains(ExtDep.Value.IncludePaths[IncIndex]))
                        AllIncludes.Add(ExtDep.Value.IncludePaths[IncIndex]);
                }
            }
        }

        if (AllIncludes.Count > 0)
        {
            Sb.AppendLine($"target_include_directories({Module.Name} PUBLIC");
            for (int Index = 0; Index < AllIncludes.Count; ++Index)
            {
                Sb.Append("  ");
                Sb.AppendLine(AllIncludes[Index]);
            }
            Sb.AppendLine(")");
        }

        if (Module.Defines.Count > 0)
        {
            Sb.Append($"target_compile_definitions({Module.Name} PRIVATE");
            for (int Index = 0; Index < Module.Defines.Count; ++Index)
            {
                Sb.Append(' ');
                Sb.Append(Module.Defines[Index]);
            }
            Sb.AppendLine(")");
        }

        if (Module.Dependencies.Count > 0)
        {
            Sb.Append($"target_link_libraries({Module.Name} PUBLIC");
            for (int Index = 0; Index < Module.Dependencies.Count; ++Index)
            {
                Sb.Append(' ');
                Sb.Append(Module.Dependencies[Index]);
            }
            Sb.AppendLine(")");
        }

        if (LinkTarget != null && (LinkTarget.LibraryPaths.Count > 0 || LinkTarget.SystemLibraries.Count > 0))
        {
            if (LinkTarget.LibraryPaths.Count > 0)
            {
                Sb.AppendLine($"target_link_directories({Module.Name} PUBLIC");
                for (int Index = 0; Index < LinkTarget.LibraryPaths.Count; ++Index)
                {
                    Sb.Append("  ");
                    Sb.AppendLine(LinkTarget.LibraryPaths[Index]);
                }
                Sb.AppendLine(")");
            }

            if (LinkTarget.SystemLibraries.Count > 0)
            {
                Sb.Append($"target_link_libraries({Module.Name} PUBLIC");
                for (int Index = 0; Index < LinkTarget.SystemLibraries.Count; ++Index)
                {
                    Sb.Append(' ');
                    Sb.Append(LinkTarget.SystemLibraries[Index]);
                }
                Sb.AppendLine(")");
            }
        }
        else if (PlatformLibs.Count > 0)
        {
            Sb.Append($"target_link_libraries({Module.Name} PUBLIC");
            for (int Index = 0; Index < PlatformLibs.Count; ++Index)
            {
                Sb.Append(' ');
                Sb.Append(PlatformLibs[Index]);
            }
            Sb.AppendLine(")");
        }
    }
}
