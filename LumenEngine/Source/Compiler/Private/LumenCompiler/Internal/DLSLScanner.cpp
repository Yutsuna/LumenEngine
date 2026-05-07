/**
 * @file DLSLScanner.cpp
 * @brief Implementation of the DLSL lexical scanner.
 */

#include "LumenCompiler/Internal/DLSLScanner.hpp"

#include <cctype>

namespace
{

inline LumenEngine::Bool IsDigit ( LumenEngine::AnsiChar Character ) noexcept
{
    return std::isdigit( Character ) != 0;
}

inline LumenEngine::Bool IsAlpha ( LumenEngine::AnsiChar Character ) noexcept
{
    return std::isalpha( Character ) != 0;
}

inline LumenEngine::Bool IsAlphaNumeric ( LumenEngine::AnsiChar Character ) noexcept
{
    return IsAlpha( Character ) or IsDigit( Character );
}

} // namespace

/**
 * FToken
 */

LumenEngine::Compiler::FToken LumenEngine::Compiler::FToken::Make ( EType InType, FStringView InText, USize InLine ) noexcept
{
    return FToken{ .Type = InType, .Text = InText, .Line = InLine };
}

/**
 * FDLSLScanner
 */

LumenEngine::Compiler::FDLSLScanner::FDLSLScanner ( FStringView InSource ) noexcept : Source( InSource )
{
    /* Ctor */
}

LumenEngine::Bool LumenEngine::Compiler::FDLSLScanner::IsAtEnd () const noexcept
{
    return Cursor >= Source.size();
}

LumenEngine::AnsiChar LumenEngine::Compiler::FDLSLScanner::Peek () const noexcept
{
    if ( IsAtEnd() )
    {
        return '\0';
    }

    return Source.at( Cursor );
}

LumenEngine::AnsiChar LumenEngine::Compiler::FDLSLScanner::Advance () noexcept
{
    if ( IsAtEnd() )
    {
        return '\0';
    }

    const AnsiChar Character = Source.at( Cursor );
    ++Cursor;

    return Character;
}

void LumenEngine::Compiler::FDLSLScanner::SkipWhitespace () noexcept
{
    while ( not IsAtEnd() )
    {
        const AnsiChar Character = Peek();

        switch ( Character )
        {
        case ' ':
        case '\r':
        case '\t':
            Advance();
            break;

        case '\n':
            ++CurrentLine;
            Advance();
            break;

        case '/':
        {
            while ( not IsAtEnd() and Peek() != '\n' )
            {
                Advance();
            }
        }
        break;

        default:
            return;
        }
    }
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLScanner::ScanToken () noexcept
{
    SkipWhitespace();

    if ( IsAtEnd() )
    {
        return FToken::Make( FToken::EType::EndOfFile, "", CurrentLine );
    }

    const USize StartIndex   = Cursor;
    const AnsiChar Character = Advance();

    switch ( Character )
    {
    case '@':
        return FToken::Make( FToken::EType::At, Source.substr( StartIndex, 1 ), CurrentLine );
    case '{':
        return FToken::Make( FToken::EType::LBrace, Source.substr( StartIndex, 1 ), CurrentLine );
    case '}':
        return FToken::Make( FToken::EType::RBrace, Source.substr( StartIndex, 1 ), CurrentLine );
    case '[':
        return FToken::Make( FToken::EType::LBracket, Source.substr( StartIndex, 1 ), CurrentLine );
    case ']':
        return FToken::Make( FToken::EType::RBracket, Source.substr( StartIndex, 1 ), CurrentLine );
    case '(':
        return FToken::Make( FToken::EType::LParen, Source.substr( StartIndex, 1 ), CurrentLine );
    case ')':
        return FToken::Make( FToken::EType::RParen, Source.substr( StartIndex, 1 ), CurrentLine );
    case ':':
        return FToken::Make( FToken::EType::Colon, Source.substr( StartIndex, 1 ), CurrentLine );
    case '=':
        return FToken::Make( FToken::EType::Equals, Source.substr( StartIndex, 1 ), CurrentLine );
    case ',':
        return FToken::Make( FToken::EType::Comma, Source.substr( StartIndex, 1 ), CurrentLine );
    case '"':
        return ScanStringToken( StartIndex );
    default:
        break;
    }

    if ( IsAlpha( Character ) or Character == '_' )
    {
        return ScanIdentifierToken( StartIndex );
    }

    if ( IsDigit( Character ) or ( Character == '-' and IsDigit( Peek() ) ) )
    {
        return ScanNumberToken( StartIndex );
    }

    return FToken::Make( FToken::EType::Unknown, Source.substr( StartIndex, 1 ), CurrentLine );
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLScanner::ScanStringToken ( USize InStartIndex ) noexcept
{
    while ( not IsAtEnd() and Peek() != '"' and Peek() != '\n' )
    {
        Advance();
    }

    if ( IsAtEnd() or Peek() == '\n' )
    {
        return FToken::Make( FToken::EType::Unknown, Source.substr( InStartIndex, Cursor - InStartIndex ), CurrentLine );
    }

    Advance();

    return FToken::Make( FToken::EType::String, Source.substr( InStartIndex + 1, Cursor - InStartIndex - 2 ), CurrentLine );
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLScanner::ScanIdentifierToken ( USize InStartIndex ) noexcept
{
    while ( IsAlphaNumeric( Peek() ) or Peek() == '_' )
    {
        Advance();
    }

    return FToken::Make( FToken::EType::Identifier, Source.substr( InStartIndex, Cursor - InStartIndex ), CurrentLine );
}

LumenEngine::Compiler::FToken LumenEngine::Compiler::FDLSLScanner::ScanNumberToken ( USize InStartIndex ) noexcept
{
    while ( IsDigit( Peek() ) )
    {
        Advance();
    }

    if ( Peek() == '.' )
    {
        Advance();
        while ( IsDigit( Peek() ) )
        {
            Advance();
        }
    }

    return FToken::Make( FToken::EType::Number, Source.substr( InStartIndex, Cursor - InStartIndex ), CurrentLine );
}
