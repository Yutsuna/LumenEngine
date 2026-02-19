using System.Text;
using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Abstract compiler interface with flag-generation API.
/// Subclasses provide tool paths; common flag logic lives here.
/// </summary>
public abstract class Compiler
{
    public abstract string Name { get; }
    public abstract string CompilerPath { get; }
    public abstract string LinkerPath { get; }
    public abstract string ArchiverPath { get; }

    public virtual string IncludePrefix => "-I";
    public virtual string DefinePrefix => "-D";
    public virtual string LibraryPathPrefix => "-L";
    public virtual string LibraryPrefix => "-l";
    public virtual string SharedFlag => "-shared";
    public virtual string PositionIndependentFlag => "-fPIC";
    public virtual string StandardFlag => "-std=c++23";
    public virtual string ArchiveFlags => "rcs";

    /// <summary>
    /// Returns base compile flags shared across all configurations.
    /// </summary>
    public virtual string GetBaseCompileFlags()
    {
        return $"{StandardFlag} {PositionIndependentFlag}";
    }

    /// <summary>
    /// Returns configuration-specific compiler flags.
    /// </summary>
    public virtual string GetConfigurationFlags(BuildConfiguration Config)
    {
        return Config switch
        {
            BuildConfiguration.Debug => $"-g -O0 {DefinePrefix}DEBUG",
            BuildConfiguration.Development => $"-g -O2 {DefinePrefix}NDEBUG",
            BuildConfiguration.Release => $"-O3 {DefinePrefix}NDEBUG",
            _ => ""
        };
    }

    /// <summary>
    /// Returns the full set of compile flags for a given configuration.
    /// </summary>
    public string GetCompileFlags(BuildConfiguration Config)
    {
        string ConfigFlags = GetConfigurationFlags(Config);
        string BaseFlags = GetBaseCompileFlags();
        return string.IsNullOrEmpty(ConfigFlags)
            ? BaseFlags
            : $"{ConfigFlags} {BaseFlags}";
    }

    public virtual string GetCompileCommand(
        string SourceFile,
        string ObjectFile,
        IReadOnlyList<string> Includes,
        IReadOnlyList<string> Defines,
        BuildConfiguration Config)
    {
        var Sb = new StringBuilder();

        Sb.Append(CompilerPath);
        Sb.Append(" -c ");
        Sb.Append(GetCompileFlags(Config));

        CompilerHelpers.AppendPrefixedQuoted(Sb, Includes, IncludePrefix);
        CompilerHelpers.AppendPrefixed(Sb, Defines, DefinePrefix);

        Sb.Append(" -o \"");
        Sb.Append(ObjectFile);
        Sb.Append("\" \"");
        Sb.Append(SourceFile);
        Sb.Append('"');

        return Sb.ToString();
    }

    public virtual string GetLinkCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles,
        IReadOnlyList<string> Libraries,
        bool IsShared)
    {
        var Sb = new StringBuilder();
        Sb.Append(LinkerPath);

        if (IsShared)
        {
            Sb.Append(' ');
            Sb.Append(SharedFlag);
        }

        Sb.Append(" -o \"");
        Sb.Append(OutputFile);
        Sb.Append('"');

        CompilerHelpers.AppendQuotedPaths(Sb, ObjectFiles);
        CompilerHelpers.AppendPrefixed(Sb, Libraries, LibraryPrefix);

        return Sb.ToString();
    }

    public virtual string GetArchiveCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles)
    {
        var Sb = new StringBuilder();
        Sb.Append(ArchiverPath);
        Sb.Append(' ');
        Sb.Append(ArchiveFlags);
        Sb.Append(" \"");
        Sb.Append(OutputFile);
        Sb.Append('"');

        CompilerHelpers.AppendQuotedPaths(Sb, ObjectFiles);

        return Sb.ToString();
    }
}

/// <summary>
/// Helper methods for building compiler command strings.
/// </summary>
public static class CompilerHelpers
{
    public static void AppendPrefixedQuoted(StringBuilder Sb, IReadOnlyList<string> Values, string Prefix)
    {
        for (int Index = 0; Index < Values.Count; ++Index)
        {
            Sb.Append(' ');
            Sb.Append(Prefix);
            Sb.Append('"');
            Sb.Append(Values[Index]);
            Sb.Append('"');
        }
    }

    public static void AppendPrefixed(StringBuilder Sb, IReadOnlyList<string> Values, string Prefix)
    {
        for (int Index = 0; Index < Values.Count; ++Index)
        {
            Sb.Append(' ');
            Sb.Append(Prefix);
            Sb.Append(Values[Index]);
        }
    }

    public static void AppendQuotedPaths(StringBuilder Sb, IReadOnlyList<string> Paths)
    {
        for (int Index = 0; Index < Paths.Count; ++Index)
        {
            Sb.Append(" \"");
            Sb.Append(Paths[Index]);
            Sb.Append('"');
        }
    }
}
