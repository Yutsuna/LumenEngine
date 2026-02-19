using LumenBuilder.Common;

namespace LumenBuilder.IO;

/// <summary>
/// Scans directories for .build files. Pure filesystem operations.
/// </summary>
public sealed class FileScanner
{
    private readonly DiagnosticBag Diagnostics;

    /// <summary>
    /// Creates a new file scanner.
    /// </summary>
    public FileScanner(DiagnosticBag Diagnostics)
    {
        this.Diagnostics = Diagnostics;
    }

    /// <summary>
    /// Finds all build files under the given root directories.
    /// </summary>
    public IEnumerable<string> FindBuildFiles(params string[] Roots)
    {
        var Results = new List<string>();

        for (int RootsIndex = 0; RootsIndex < Roots.Length; RootsIndex++)
        {
            string Root = Roots[RootsIndex];
            if (!Directory.Exists(Root))
            {
                Diagnostics.Warning($"Directory not found: {Root}");
                continue;
            }

            ScanDirectory(Root, Results);
        }

        return Results;
    }

    /// <summary>
    /// Recursively scans a directory for build files.
    /// </summary>
    private void ScanDirectory(string Directory, List<string> Results)
    {
        try
        {
            string[] Files = System.IO.Directory.GetFiles(Directory, "*" + Paths.BuildFileExtension);
            for (int FilesIndex = 0; FilesIndex < Files.Length; ++FilesIndex)
            {
                Results.Add(Paths.Normalize(Files[FilesIndex]));
            }

            string[] Subdirs = System.IO.Directory.GetDirectories(Directory);
            for (int SubdIndex = 0; SubdIndex < Subdirs.Length; ++SubdIndex)
            {
                ScanDirectory(Subdirs[SubdIndex], Results);
            }
        }
        catch (UnauthorizedAccessException)
        {
            Diagnostics.Warning($"Access denied: {Directory}");
        }
        catch (IOException Ex)
        {
            Diagnostics.Warning($"IO error scanning {Directory}: {Ex.Message}");
        }
    }

}
