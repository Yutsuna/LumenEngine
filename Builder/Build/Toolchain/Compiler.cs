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
