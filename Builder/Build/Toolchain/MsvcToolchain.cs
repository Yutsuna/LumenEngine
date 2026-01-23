using System.Text;
using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// MSVC toolchain implementation.
/// </summary>
public sealed class MsvcToolchain : Compiler
{
    public override string Name => "msvc";
    public override string CompilerPath => "cl.exe";
    public override string LinkerPath => "link.exe";
    public override string ArchiverPath => "lib.exe";

    public override string GetCompileCommand(
        string SourceFile,
        string ObjectFile,
        IReadOnlyList<string> Includes,
        IReadOnlyList<string> Defines,
        BuildConfiguration Config)
    {
        var Sb = new StringBuilder();
        Sb.Append(CompilerPath);
        Sb.Append(" /c /EHsc /std:c++23 /nologo");

        switch (Config)
        {
            case BuildConfiguration.Debug:
                Sb.Append(" /Zi /Od /MDd /DDEBUG");
                break;
            case BuildConfiguration.Development:
                Sb.Append(" /Zi /O2 /MD /DNDEBUG");
                break;
            case BuildConfiguration.Release:
                Sb.Append(" /O2 /MD /DNDEBUG");
                break;
        }

        for (int I = 0; I < Includes.Count; I++)
        {
            Sb.Append(" /I\"");
            Sb.Append(Includes[I]);
            Sb.Append('"');
        }

        for (int I = 0; I < Defines.Count; I++)
        {
            Sb.Append(" /D");
            Sb.Append(Defines[I]);
        }

        Sb.Append(" /Fo\"");
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
        Sb.Append(" /nologo");

        if (IsShared)
            Sb.Append(" /DLL");

        Sb.Append(" /OUT:\"");
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
            Sb.Append(' ');
            Sb.Append(Libraries[I]);
            Sb.Append(".lib");
        }

        return Sb.ToString();
    }

    public override string GetArchiveCommand(
        string OutputFile,
        IReadOnlyList<string> ObjectFiles)
    {
        var Sb = new StringBuilder();
        Sb.Append(ArchiverPath);
        Sb.Append(" /nologo /OUT:\"");
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
