namespace LumenBuilder.Parsing.Lexer;

/// <summary>
/// Immutable token representation.
/// </summary>
public readonly record struct Token(TokenType Type, ReadOnlyMemory<char> Value, int Line, int Column)
{
    /// <summary>
    /// Gets the string value of the token.
    /// </summary>
    public string GetString() => new string(Value.Span);

    /// <summary>
    /// Returns a string representation of the token.
    /// </summary>
    public override string ToString() => $"[{Type}] '{GetString()}' at ({Line},{Column})";
}
