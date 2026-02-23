using LumenBuilder.Model;

namespace LumenBuilder.Common;

/// <summary>
/// Expands source file patterns into concrete file paths with recursive search support.
/// </summary>
public static class SourceExpander
{
    public static List<string> Expand(ModuleDescriptor Module)
    {
        var SourceFiles = new List<string>();

        for (int PatternIndex = 0; PatternIndex < Module.Sources.Count; ++PatternIndex)
        {
            string Pattern = Module.Sources[PatternIndex];
            string FullPattern = Paths.Combine(Module.Directory, Pattern);

            if (Pattern.Contains("**"))
            {
                ExpandRecursive(Module.Directory, Pattern, SourceFiles);
            }
            else if (Pattern.Contains('*'))
            {
                ExpandGlob(FullPattern, SourceFiles);
            }
            else
            {
                SourceFiles.Add(FullPattern);
            }
        }

        return SourceFiles;
    }

    private static void ExpandRecursive(string ModuleDir, string Pattern, List<string> Results)
    {
        string BaseDir = ModuleDir;
        string SearchPattern = Path.GetFileName(Pattern);

        int RecursiveIndex = Pattern.IndexOf("**");
        if (RecursiveIndex > 0)
        {
            string BasePath = Pattern[..RecursiveIndex].TrimEnd('/', '\\');
            BaseDir = Paths.Combine(ModuleDir, BasePath);

            string Remaining = Pattern[(RecursiveIndex + 2)..].TrimStart('/', '\\');
            SearchPattern = Remaining;
        }

        if (!Directory.Exists(BaseDir))
            return;

        string[] Files = Directory.GetFiles(BaseDir, SearchPattern, SearchOption.AllDirectories);
        for (int FileIndex = 0; FileIndex < Files.Length; ++FileIndex)
        {
            Results.Add(Paths.Normalize(Files[FileIndex]));
        }
    }

    private static void ExpandGlob(string FullPattern, List<string> Results)
    {
        string Dir = Paths.GetDirectory(FullPattern);
        string FilePattern = Path.GetFileName(FullPattern);

        if (!Directory.Exists(Dir))
            return;

        string[] Files = Directory.GetFiles(Dir, FilePattern);
        for (int FileIndex = 0; FileIndex < Files.Length; ++FileIndex)
        {
            Results.Add(Paths.Normalize(Files[FileIndex]));
        }
    }
}
