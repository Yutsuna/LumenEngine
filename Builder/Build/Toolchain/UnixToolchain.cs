using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Unix toolchain implementation parameterized by compiler name and paths.
/// Covers both g++ and clang++ since their CLI interfaces are identical.
/// </summary>
public sealed class UnixToolchain : Compiler
{
    public override string Name { get; }
    public override string CompilerPath { get; }
    public override string LinkerPath { get; }
    public override string ArchiverPath => "ar";

    public UnixToolchain(string Name, string CompilerPath)
    {
        this.Name = Name;
        this.CompilerPath = CompilerPath;
        this.LinkerPath = CompilerPath;
    }

    public static UnixToolchain Gpp() => new("g++", "g++");
    public static UnixToolchain Clang() => new("clang", "clang++");
}
