namespace LumenBuilder.Common;

/// <summary>
/// Centralized path utilities. No I/O operations.
/// </summary>
public static class Paths
{
    public const string BuildFileExtension = ".build";
    public const string IntermediateDir = "Intermediate";
    public const string BinariesDir = "Binaries";

    /// <summary>
    /// Normalizes a path by replacing backslashes with forward slashes.
    /// </summary>
    public static string Normalize(string Path)
    {
        if (string.IsNullOrEmpty(Path))
        {
            return Path;
        }

        Span<char> Buffer = stackalloc char[Path.Length];
        for (int BufIndex = 0; BufIndex < Path.Length; ++BufIndex)
        {
            Buffer[BufIndex] = Path[BufIndex] == '\\' ? '/' : Path[BufIndex];
        }

        return new string(Buffer);
    }

    /// <summary>
    /// Extracts the module name from a build file path.
    /// </summary>
    public static string GetModuleName(string BuildFilePath)
    {
        ReadOnlySpan<char> Span = BuildFilePath.AsSpan();
        int LastSep = Span.LastIndexOfAny('/', '\\');

        if (LastSep >= 0)
        {
            Span = Span[(LastSep + 1)..];
        }

        if (Span.EndsWith(BuildFileExtension.AsSpan(), StringComparison.OrdinalIgnoreCase))
        {
            Span = Span[..^BuildFileExtension.Length];
        }

        return new string(Span);
    }

    /// <summary>
    /// Combines two paths, ensuring proper separators.
    /// </summary>
    public static string Combine(ReadOnlySpan<char> BasePath, ReadOnlySpan<char> RelativePath)
    {
        if (RelativePath.IsEmpty)
        {
            return new string(BasePath);
        }
        if (BasePath.IsEmpty)
        {
            return new string(RelativePath);
        }

        char LastChar = BasePath[^1];
        bool NeedsSep = LastChar != '/' && LastChar != '\\';

        int Length = BasePath.Length + (NeedsSep ? 1 : 0) + RelativePath.Length;
        Span<char> Result = stackalloc char[Length];

        BasePath.CopyTo(Result);
        int Pos = BasePath.Length;

        if (NeedsSep)
        {
            Result[Pos++] = '/';
        }

        RelativePath.CopyTo(Result[Pos..]);

        return Normalize(new string(Result));
    }

    /// <summary>
    /// Gets the directory part of a path.
    /// </summary>
    public static string GetDirectory(string Path)
    {
        int LastSep = Path.LastIndexOfAny(new[] { '/', '\\' });

        return LastSep > 0 ? Path[..LastSep] : ".";
    }

    /// <summary>
    /// Gets the intermediate path for a given module.
    /// </summary>
    public static string GetIntermediatePath(string RootDir, string ModuleName) =>
        Combine(RootDir, Combine(IntermediateDir, ModuleName));

    /// <summary>
    /// Gets the binary path for a given module.
    /// </summary>
    public static string GetBinaryPath(string RootDir, string ModuleName) =>
        Combine(RootDir, Combine(BinariesDir, ModuleName));
}
