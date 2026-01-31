using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Abstract compiler interface.
/// </summary>
public abstract class Compiler
{
    public abstract string Name { get; }
    public abstract string CompilerPath { get; }
    public abstract string LinkerPath { get; }
    public abstract string ArchiverPath { get; }

    public abstract string GetCompileCommand(
        string SourceFile,
        string ObjectFile,
        IReadOnlyList<string> Includes,
        IReadOnlyList<string> Defines,
        BuildConfiguration Config
    );

    public abstract string GetLinkCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles,
        IReadOnlyList<string> Libraries,
        bool IsShared
    );

    public abstract string GetArchiveCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles
    );
}

/// <summary>
/// Helper methods for compiler command generation.
/// </summary>
public static partial class CompilerHelpers
{
    public static void AppendIncludes(System.Text.StringBuilder sb, IReadOnlyList<string> includes, string prefix, bool quoted)
    {
        for (int IncludeIndex = 0; IncludeIndex < includes.Count; ++IncludeIndex)
        {
            sb.Append(' ');
            sb.Append(prefix);
            if (quoted) sb.Append('"');
            sb.Append(includes[IncludeIndex]);
            if (quoted) sb.Append('"');
        }
    }

    public static void AppendDefines(System.Text.StringBuilder sb, IReadOnlyList<string> defines, string prefix)
    {
        for (int DefineIndex = 0; DefineIndex < defines.Count; ++DefineIndex)
        {
            sb.Append(' ');
            sb.Append(prefix);
            sb.Append(defines[DefineIndex]);
        }
    }

    public static void AppendQuotedPaths(System.Text.StringBuilder sb, IReadOnlyList<string> paths)
    {
        for (int PathIndex = 0; PathIndex < paths.Count; ++PathIndex)
        {
            sb.Append(' ');
            sb.Append('"');
            sb.Append(paths[PathIndex]);
            sb.Append('"');
        }
    }

    public static void AppendPrefixedValues(System.Text.StringBuilder sb, IReadOnlyList<string> values, string prefix)
    {
        for (int ValueIndex = 0; ValueIndex < values.Count; ++ValueIndex)
        {
            sb.Append(' ');
            sb.Append(prefix);
            sb.Append(values[ValueIndex]);
        }
    }

    public static void AppendLibrariesWithExtension(System.Text.StringBuilder sb, IReadOnlyList<string> libs, string extension)
    {
        for (int LibIndex = 0; LibIndex < libs.Count; ++LibIndex)
        {
            sb.Append(' ');
            sb.Append(libs[LibIndex]);
            sb.Append(extension);
        }
    }

}
