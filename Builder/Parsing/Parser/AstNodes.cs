namespace LumenBuilder.Parsing.Parser;

/// <summary>
/// Abstract syntax tree nodes for build files.
/// </summary>
public abstract record AstNode(int Line, int Column);

/// <summary>
/// Root module declaration.
/// </summary>
public sealed record ModuleNode(
    string Name,
    int Line,
    int Column,
    IReadOnlyList<PropertyNode> Properties
) : AstNode(Line, Column);

/// <summary>
/// A property assignment (e.g., type = executable).
/// </summary>
public sealed record PropertyNode(
    string Name,
    AstNode Value,
    int Line,
    int Column
) : AstNode(Line, Column);

/// <summary>
/// A simple identifier value.
/// </summary>
public sealed record IdentifierNode(
    string Value,
    int Line,
    int Column
) : AstNode(Line, Column);

/// <summary>
/// A string literal value.
/// </summary>
public sealed record StringNode(
    string Value,
    int Line,
    int Column
) : AstNode(Line, Column);

/// <summary>
/// An array of values.
/// </summary>
public sealed record ArrayNode(
    IReadOnlyList<AstNode> Elements,
    int Line,
    int Column
) : AstNode(Line, Column);
