namespace LumenBuilder.Common;

/// <summary>
/// Exception thrown when a build error occurs.
/// </summary>
public sealed class BuildException : Exception
{
    public string? FilePath { get; }
    public int Line { get; }
    public int Column { get; }

    /// <summary>
    /// Creates a new build exception.
    /// </summary>
    public BuildException(string Message)
        : base(Message)
    {
        FilePath = null;
        Line = 0;
        Column = 0;
    }

    /// <summary>
    /// Creates a new build exception with file and location information.
    /// </summary>
    public BuildException(string Message, string FilePath, int Line, int Column)
        : base($"{FilePath}({Line},{Column}): {Message}")
    {
        this.FilePath = FilePath;
        this.Line = Line;
        this.Column = Column;
    }

    /// <summary>
    /// Creates a new build exception with an inner exception.
    /// </summary>
    public BuildException(string Message, Exception InnerException)
        : base(Message, InnerException)
    {
        FilePath = null;
        Line = 0;
        Column = 0;
    }

}
