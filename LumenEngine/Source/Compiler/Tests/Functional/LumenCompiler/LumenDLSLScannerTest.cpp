/**
 * @file LumenDLSLScannerTest.cpp
 * @brief Tests for the DLSL scanner.
 */

#include "LumenCompilerTest.hpp"

namespace LumenEngine
{

TEST( FDLSLScanner, AtToken )
{
    Compiler::FDLSLScanner Scanner( "@Mesh" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::At );
    EXPECT_EQ( T.Text, "@" );

    T = Scanner.ScanToken();
    EXPECT_EQ( T.Type, Compiler::FToken::EType::Identifier );
    EXPECT_EQ( T.Text, "Mesh" );
}

TEST( FDLSLScanner, StringToken )
{
    Compiler::FDLSLScanner Scanner( R"("Hello World")" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::String );
    EXPECT_EQ( T.Text, "Hello World" );
}

TEST( FDLSLScanner, NumberIntegerToken )
{
    Compiler::FDLSLScanner Scanner( "42" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::Number );
    EXPECT_EQ( T.Text, "42" );
}

TEST( FDLSLScanner, NumberFloatToken )
{
    Compiler::FDLSLScanner Scanner( "3.14" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::Number );
    EXPECT_EQ( T.Text, "3.14" );
}

TEST( FDLSLScanner, NegativeNumberToken )
{
    Compiler::FDLSLScanner Scanner( "-1.5" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::Number );
    EXPECT_EQ( T.Text, "-1.5" );
}

TEST( FDLSLScanner, PunctuationTokens )
{
    Compiler::FDLSLScanner Scanner( "{ } [ ] ( ) : = ," );
    constexpr Compiler::FToken::EType Expected[] = { Compiler::FToken::EType::LBrace,   Compiler::FToken::EType::RBrace, Compiler::FToken::EType::LBracket,
                                                     Compiler::FToken::EType::RBracket, Compiler::FToken::EType::LParen, Compiler::FToken::EType::RParen,
                                                     Compiler::FToken::EType::Colon,    Compiler::FToken::EType::Equals, Compiler::FToken::EType::Comma };

    for ( const Compiler::FToken::EType ExpectedType : Expected )
    {
        Compiler::FToken T = Scanner.ScanToken();
        EXPECT_EQ( T.Type, ExpectedType );
    }
}

TEST( FDLSLScanner, LineCommentIsSkipped )
{
    Compiler::FDLSLScanner Scanner( "// this is a comment\nIdentifier" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::Identifier );
    EXPECT_EQ( T.Text, "Identifier" );
    EXPECT_EQ( T.Line, 2U );
}

TEST( FDLSLScanner, UnterminatedStringIsUnknown )
{
    Compiler::FDLSLScanner Scanner( "\"unterminated" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::Unknown );
}

TEST( FDLSLScanner, EndOfFileToken )
{
    Compiler::FDLSLScanner Scanner( "" );
    Compiler::FToken T = Scanner.ScanToken();

    EXPECT_EQ( T.Type, Compiler::FToken::EType::EndOfFile );
}

TEST( FDLSLScanner, BooleanIdentifiers )
{
    Compiler::FDLSLScanner Scanner( "True False" );
    Compiler::FToken T1 = Scanner.ScanToken();

    EXPECT_EQ( T1.Type, Compiler::FToken::EType::Identifier );
    EXPECT_EQ( T1.Text, "True" );

    Compiler::FToken T2 = Scanner.ScanToken();

    EXPECT_EQ( T2.Type, Compiler::FToken::EType::Identifier );
    EXPECT_EQ( T2.Text, "False" );
}

TEST( FDLSLScanner, LineNumberTracking )
{
    Compiler::FDLSLScanner Scanner( "a\nb\nc" );

    Compiler::FToken T1 = Scanner.ScanToken();
    EXPECT_EQ( T1.Line, 1U );

    Compiler::FToken T2 = Scanner.ScanToken();
    EXPECT_EQ( T2.Line, 2U );

    Compiler::FToken T3 = Scanner.ScanToken();
    EXPECT_EQ( T3.Line, 3U );
}

} // namespace LumenEngine
