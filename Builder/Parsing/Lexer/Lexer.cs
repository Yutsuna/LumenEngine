using LumenBuilder.Common;

namespace LumenBuilder.Parsing.Lexer;

/// <summary>
/// Pure Lexer for build files
/// </summary>
public sealed class BuildLexer
{
    private readonly ReadOnlyMemory<char> Source;
    private int Position;
    private int Line;
    private int Column;

    /// <summary>
    /// Constructs a new BuildLexer
    /// </summary>
    public BuildLexer(ReadOnlyMemory<char> Source)
    {
        this.Source = Source;
        Position = 0;
        Line = 1;
        Column = 1;
    }

    /// <summary>
    /// Gets the next token from the source
    /// </summary>
    public Token NextToken()
    {
        SkipWhitespaceAndComments();

        if (Position >= Source.Length)
        {
            return new Token(TokenType.EndOfFile, ReadOnlyMemory<char>.Empty, Line, Column);
        }

        int StartLine = Line;
        int StartColumn = Column;
        char Current = Source.Span[Position];

        if (char.IsLetter(Current) || Current == '_')
        {
            return ReadIdentifierOrKeyword(StartLine, StartColumn);
        }

        if (Current == '"')
        {
            return ReadString(StartLine, StartColumn);
        }

        return ReadSymbol(StartLine, StartColumn);
    }

    /// <summary>
    /// Tokenizes the entire source into a list of tokens
    /// </summary>
    public List<Token> Tokenize()
    {
        var Tokens = new List<Token>();
        Token Tok;

        do
        {
            Tok = NextToken();
            Tokens.Add(Tok);
        } while (Tok.Type != TokenType.EndOfFile);

        return Tokens;
    }

    /// <summary>
    /// Skips whitespace and comments in the source
    /// </summary>
    private void SkipWhitespaceAndComments()
    {
        while (Position < Source.Length)
        {
            char Current = Source.Span[Position];

            if (char.IsWhiteSpace(Current))
            {
                if (Current == '\n')
                {
                    ++Line;
                    Column = 1;
                }
                else
                {
                    ++Column;
                }
                ++Position;
                continue;
            }

            if (Current == '/' && Position + 1 < Source.Length)
            {
                char Next = Source.Span[Position + 1];

                if (Next == '/')
                {
                    SkipLineComment();
                    continue;
                }
                if (Next == '*')
                {
                    SkipBlockComment();
                    continue;
                }
            }

            break;
        }
    }

    /// <summary>
    /// Skips a line comment in the source
    /// </summary>
    private void SkipLineComment()
    {
        while (Position < Source.Length && Source.Span[Position] != '\n')
        {
            ++Position;
        }
    }

    /// <summary>
    /// Skips a block comment in the source
    /// </summary>
    private void SkipBlockComment()
    {
        Position += 2;
        Column += 2;

        while (Position + 1 < Source.Length)
        {
            if (Source.Span[Position] == '*' && Source.Span[Position + 1] == '/')
            {
                Position += 2;
                Column += 2;
                return;
            }
            if (Source.Span[Position] == '\n')
            {
                ++Line;
                Column = 1;
            }
            else
            {
                ++Column;
            }
            ++Position;
        }
        throw new BuildException("Unterminated block comment", "", Line, Column);
    }

    /// <summary>
    /// Reads an identifier or keyword from the source
    /// </summary>
    private Token ReadIdentifierOrKeyword(int StartLine, int StartColumn)
    {
        int Start = Position;

        while (Position < Source.Length)
        {
            char C = Source.Span[Position];

            if (!char.IsLetterOrDigit(C) && C != '_')
            {
                break;
            }

            ++Position;
            ++Column;
        }

        ReadOnlyMemory<char> Value = Source.Slice(Start, Position - Start);
        TokenType Type = GetKeywordType(Value.Span);

        return new Token(Type, Value, StartLine, StartColumn);
    }

    /// <summary>
    /// Gets the token type for a given keyword
    /// </summary>
    private static TokenType GetKeywordType(ReadOnlySpan<char> Value)
    {
        return Value switch
        {
            "module" => TokenType.Module,
            "type" => TokenType.Type,
            "sources" => TokenType.Sources,
            "public_includes" => TokenType.PublicIncludes,
            "private_includes" => TokenType.PrivateIncludes,
            "defines" => TokenType.Defines,
            "deps" => TokenType.Deps,
            _ => TokenType.Identifier
        };
    }

    /// <summary>
    /// Reads a string literal from the source
    /// </summary>
    private Token ReadString(int StartLine, int StartColumn)
    {
        ++Position;
        ++Column;

        int Start = Position;

        while (Position < Source.Length && Source.Span[Position] != '"')
        {
            if (Source.Span[Position] == '\n')
            {
                throw new BuildException("Unterminated string", "", StartLine, StartColumn);
            }

            ++Position;
            ++Column;
        }

        if (Position >= Source.Length)
        {
            throw new BuildException("Unterminated string", "", StartLine, StartColumn);
        }

        ReadOnlyMemory<char> Value = Source.Slice(Start, Position - Start);

        ++Position;
        ++Column;

        return new Token(TokenType.String, Value, StartLine, StartColumn);
    }

    /// <summary>
    /// Reads a symbol token from the source
    /// </summary>
    private Token ReadSymbol(int StartLine, int StartColumn)
    {
        char Current = Source.Span[Position];

        ++Position;
        ++Column;

        TokenType Type = Current switch
        {
            '{' => TokenType.LeftBrace,
            '}' => TokenType.RightBrace,
            '[' => TokenType.LeftBracket,
            ']' => TokenType.RightBracket,
            '=' => TokenType.Equals,
            ',' => TokenType.Comma,
            _ => TokenType.Invalid
        };

        return new Token(Type, Source.Slice(Position - 1, 1), StartLine, StartColumn);
    }

}
