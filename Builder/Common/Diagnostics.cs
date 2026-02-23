namespace LumenBuilder.Common;

/// <summary>
/// Diagnostic severity levels.
/// </summary>
public enum DiagnosticSeverity
{
    Info,
    Warning,
    Error
}

/// <summary>
/// Represents a single diagnostic message.
/// </summary>
public readonly record struct Diagnostic(
    DiagnosticSeverity Severity,
    string Message,
    string? FilePath = null,
    int Line = 0,
    int Column = 0
)
{

    /// <summary>
    /// Returns a string representation of the diagnostic.
    /// </summary>
    public override string ToString()
    {
        if (FilePath is null)
        {
            return $"[{Severity}] {Message}";
        }

        return $"{FilePath}({Line},{Column}): [{Severity}] {Message}";
    }

}

/// <summary>
/// Thread-safe diagnostic collector.
/// </summary>
public sealed class DiagnosticBag
{
    private readonly object Lock = new();
    private readonly List<Diagnostic> Diagnostics = new();

    /// <summary>
    /// Adds a diagnostic to the bag.
    /// </summary>
    public void Add(Diagnostic Diag)
    {
        lock (Lock)
        {
            Diagnostics.Add(Diag);
        }
    }

    /// <summary>
    /// Adds an info diagnostic.
    /// </summary>
    public void Info(string Message) =>
        Add(new Diagnostic(DiagnosticSeverity.Info, Message));

    /// <summary>
    /// Adds a warning diagnostic.
    /// </summary>
    public void Warning(string Message, string? File = null, int Line = 0, int Column = 0) =>
        Add(new Diagnostic(DiagnosticSeverity.Warning, Message, File, Line, Column));

    /// <summary>
    /// Adds an error diagnostic.
    /// </summary>
    public void Error(string Message, string? File = null, int Line = 0, int Column = 0) =>
        Add(new Diagnostic(DiagnosticSeverity.Error, Message, File, Line, Column));

    /// <summary>
    /// Gets whether the bag contains any error diagnostics.
    /// </summary>
    public bool HasErrors
    {
        get
        {
            lock (Lock)
            {
                for (int DiagIndex = 0; DiagIndex < Diagnostics.Count; ++DiagIndex)
                {
                    if (Diagnostics[DiagIndex].Severity == DiagnosticSeverity.Error)
                    {
                        return true;
                    }
                }
                return false;
            }
        }
    }

    /// <summary>
    /// Gets all diagnostics in the bag.
    /// </summary>
    public IReadOnlyList<Diagnostic> GetAll()
    {
        lock (Lock)
        {
            return Diagnostics.ToArray();
        }
    }

    /// <summary>
    /// Clears all diagnostics from the bag.
    /// </summary>
    public void Clear()
    {
        lock (Lock)
        {
            Diagnostics.Clear();
        }
    }

}
