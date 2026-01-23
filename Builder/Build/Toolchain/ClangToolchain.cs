using System.Text;
using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Clang/GCC toolchain implementation.
/// </summary>
public sealed class ClangToolchain : Compiler
{
    public override string Name => "clang";
    public override string CompilerPath => "clang++";
    public override string LinkerPath => "clang++";
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

        for (int I = 0; I < Includes.Count; I++)
        {
            Sb.Append(" -I\"");
            Sb.Append(Includes[I]);
            Sb.Append('"');
        }

        for (int I = 0; I < Defines.Count; I++)
        {
            Sb.Append(" -D");
            Sb.Append(Defines[I]);
        }

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

        for (int I = 0; I < ObjectFiles.Count; I++)
        {
            Sb.Append(" \"");
            Sb.Append(ObjectFiles[I]);
            Sb.Append('"');
        }

        for (int I = 0; I < Libraries.Count; I++)
        {
            Sb.Append(" -l");
            Sb.Append(Libraries[I]);
        }

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

        for (int I = 0; I < ObjectFiles.Count; I++)
        {
            Sb.Append(" \"");
            Sb.Append(ObjectFiles[I]);
            Sb.Append('"');
        }

        return Sb.ToString();
    }
}
