using LumenBuilder.Common;
using LumenBuilder.Parsing.Lexer;

namespace LumenBuilder.Parsing.Parser;

/// <summary>
/// Parser for build files. Produces AST from tokens.
/// </summary>
public sealed class BuildParser
{
    private readonly List<Token> Tokens;
    private int Position;

    public BuildParser(List<Token> Tokens)
    {
        this.Tokens = Tokens;
        Position = 0;
    }

    /// <summary>
    /// Parses the tokens into an AST.
    /// </summary>
    public ModuleNode Parse()
    {
        return ParseModule();
    }

    /// <summary>
    /// Parses a module declaration.
    /// </summary>
    private ModuleNode ParseModule()
    {
        Token ModuleKeyword = Expect(TokenType.Module);
        Token NameToken = Expect(TokenType.Identifier);
        Expect(TokenType.LeftBrace);

        var Properties = new List<PropertyNode>();

        while (!Check(TokenType.RightBrace) && !Check(TokenType.EndOfFile))
        {
            Properties.Add(ParseProperty());
        }

        Expect(TokenType.RightBrace);

        return new ModuleNode(
            NameToken.GetString(),
            ModuleKeyword.Line,
            ModuleKeyword.Column,
            Properties
        );
    }

    /// <summary>
    /// Parses a property assignment.
    /// </summary>
    private PropertyNode ParseProperty()
    {
        Token NameToken = Current();

        if (!IsPropertyKeyword(NameToken.Type) && NameToken.Type != TokenType.Identifier)
        {
            throw new BuildException(
                $"Expected property name, got {NameToken.Type}",
                "",
                NameToken.Line,
                NameToken.Column
            );
        }

        Advance();
        Expect(TokenType.Equals);
        AstNode Value = ParseValue();

        return new PropertyNode(
            NameToken.GetString(),
            Value,
            NameToken.Line,
            NameToken.Column
        );
    }

    /// <summary>
    /// Checks if a token type is a valid property keyword.
    /// </summary>
    private static bool IsPropertyKeyword(TokenType Type)
    {
        return Type == TokenType.Type ||
               Type == TokenType.Sources ||
               Type == TokenType.PublicIncludes ||
               Type == TokenType.PrivateIncludes ||
               Type == TokenType.Defines ||
               Type == TokenType.Deps;
    }

    /// <summary>
    /// Parses a value (string, identifier, or array).
    /// </summary>
    private AstNode ParseValue()
    {
        Token Tok = Current();

        if (Tok.Type == TokenType.LeftBracket)
        {
            return ParseArray();
        }

        if (Tok.Type == TokenType.String)
        {
            Advance();
            return new StringNode(Tok.GetString(), Tok.Line, Tok.Column);
        }

        if (Tok.Type == TokenType.Identifier)
        {
            Advance();
            return new IdentifierNode(Tok.GetString(), Tok.Line, Tok.Column);
        }

        throw new BuildException(
            $"Expected value, got {Tok.Type}",
            "",
            Tok.Line,
            Tok.Column
        );
    }

    /// <summary>
    /// Parses an array value.
    /// </summary>
    private ArrayNode ParseArray()
    {
        Token Start = Expect(TokenType.LeftBracket);
        var Elements = new List<AstNode>();

        while (!Check(TokenType.RightBracket) && !Check(TokenType.EndOfFile))
        {
            Elements.Add(ParseArrayElement());

            if (Check(TokenType.Comma))
            {
                Advance();
            }
        }

        Expect(TokenType.RightBracket);

        return new ArrayNode(Elements, Start.Line, Start.Column);
    }

    /// <summary>
    /// Parses an element of an array.
    /// </summary>
    private AstNode ParseArrayElement()
    {
        Token Tok = Current();

        if (Tok.Type == TokenType.String)
        {
            Advance();
            return new StringNode(Tok.GetString(), Tok.Line, Tok.Column);
        }

        if (Tok.Type == TokenType.Identifier)
        {
            Advance();
            return new IdentifierNode(Tok.GetString(), Tok.Line, Tok.Column);
        }

        throw new BuildException(
            $"Expected array element, got {Tok.Type}",
            "",
            Tok.Line,
            Tok.Column
        );
    }

    /// <summary>
    /// Gets the current token.
    /// </summary>
    private Token Current()
    {
        if (Position >= Tokens.Count)
        {
            return Tokens[Tokens.Count - 1];
        }

        return Tokens[Position];
    }

    /// <summary>
    /// Checks if the current token matches the given type.
    /// </summary>
    private bool Check(TokenType Type) => Current().Type == Type;

    /// <summary>
    /// Advances to the next token and returns the current one.
    /// </summary>
    private Token Advance()
    {
        Token Tok = Current();

        if (Position < Tokens.Count)
        {
            ++Position;
        }

        return Tok;
    }

    /// <summary>
    /// Expects the current token to match the given type, advancing if so.
    /// </summary>
    private Token Expect(TokenType Type)
    {
        Token Tok = Current();

        if (Tok.Type != Type)
        {
            throw new BuildException(
                $"Expected {Type}, got {Tok.Type}",
                "",
                Tok.Line,
                Tok.Column
            );
        }

        return Advance();
    }

}
