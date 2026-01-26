using System.Text;
using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// G++ toolchain implementation (uses g++ as compiler and linker).
/// </summary>
public sealed class GppToolchain : Compiler
{
    public override string Name => "g++";
    public override string CompilerPath => "g++";
    public override string LinkerPath => "g++";
    public override string ArchiverPath => "ar";

    public override string GetCompileCommand(
        string SourceFile,
        string ObjectFile,
        IReadOnlyList<string> Includes,
        IReadOnlyList<string> Defines,
        BuildConfiguration Config)
    {
        var Sb = new StringBuilder();

        Sb.Append(CompilerPath);
        Sb.Append(" -c -std=c++23");

        switch (Config)
        {
            case BuildConfiguration.Debug:
                Sb.Append(" -g -O0 -DDEBUG");
                break;
            case BuildConfiguration.Development:
                Sb.Append(" -g -O2 -DNDEBUG");
                break;
            case BuildConfiguration.Release:
                Sb.Append(" -O3 -DNDEBUG");
                break;
        }

        CompilerHelpers.AppendIncludes(Sb, Includes, "-I", true);
        CompilerHelpers.AppendDefines(Sb, Defines, "-D");

        Sb.Append(" -o \"");
        Sb.Append(ObjectFile);
        Sb.Append("\" \"");
        Sb.Append(SourceFile);
        Sb.Append('"');

        return Sb.ToString();
    }

    public override string GetLinkCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles,
        IReadOnlyList<string> Libraries,
        bool IsShared)
    {
        var Sb = new StringBuilder();
        Sb.Append(LinkerPath);

        if (IsShared)
            Sb.Append(" -shared");

        Sb.Append(" -o \"");
        Sb.Append(OutputFile);
        Sb.Append('"');

        CompilerHelpers.AppendQuotedPaths(Sb, ObjectFiles);
        CompilerHelpers.AppendPrefixedValues(Sb, Libraries, "-l");

        return Sb.ToString();
    }

    public override string GetArchiveCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles)
    {
        var Sb = new StringBuilder();
        Sb.Append(ArchiverPath);
        Sb.Append(" rcs \"");
        Sb.Append(OutputFile);
        Sb.Append('"');

        CompilerHelpers.AppendQuotedPaths(Sb, ObjectFiles);

        return Sb.ToString();
    }
}
