namespace LumenBuilder.Parsing.Lexer;

/// <summary>
/// Token types for the build file lexer.
/// </summary>
public enum TokenType
{
    Module,
    Type,
    Sources,
    PublicIncludes,
    PrivateIncludes,
    Defines,
    Deps,

    Identifier,
    String,

    LeftBrace,
    RightBrace,
    LeftBracket,
    RightBracket,
    Equals,
    Comma,

    EndOfFile,
    Invalid
}
