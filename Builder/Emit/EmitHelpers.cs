namespace LumenBuilder.Emit;

/// <summary>
/// Shared utilities for build file emitters.
/// </summary>
public static class EmitHelpers
{
    /// <summary>
    /// Writes generated content to a file, creating directories as needed.
    /// </summary>
    public static void WriteToFile(string OutputPath, string Content)
    {
        string Dir = Path.GetDirectoryName(OutputPath) ?? ".";

        if (!Directory.Exists(Dir))
        {
            Directory.CreateDirectory(Dir);
        }

        File.WriteAllText(OutputPath, Content);
    }
}
