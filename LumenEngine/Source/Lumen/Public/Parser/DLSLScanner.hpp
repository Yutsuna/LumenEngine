/**
 * @file DLSLScanner.hpp
 * @brief Zero-allocation lexical scanner for DLSL files.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/String.hpp"

namespace LumenEngine
{

namespace Lumen
{

    struct FToken
    {

        enum class EType : UInt8
        {
            Unknown,
            EndOfFile,
            Identifier,
            String,
            Number,
            At,
            LBrace,
            RBrace,
            LBracket,
            RBracket,
            LParen,
            RParen,
            Colon,
            Equals,
            Comma
        };

        EType Type = EType::Unknown;
        FStringView Text;
        USize Line = 1;

        static FToken Make ( EType InType, FStringView InText, USize InLine ) noexcept;
    };

    class LUMEN_ENGINE_API FDLSLScanner final
    {
    public:

        explicit FDLSLScanner ( FStringView InSource ) noexcept;

    public:

        [[nodiscard]] FToken ScanToken () noexcept;

    private:

        void SkipWhitespace () noexcept;

        [[nodiscard]] Bool IsAtEnd () const noexcept;
        [[nodiscard]] AnsiChar Peek () const noexcept;
        AnsiChar Advance () noexcept;

    private:

        [[nodiscard]] FToken ScanStringToken ( USize InStartIndex ) noexcept;
        [[nodiscard]] FToken ScanIdentifierToken ( USize InStartIndex ) noexcept;
        [[nodiscard]] FToken ScanNumberToken ( USize InStartIndex ) noexcept;

    private:

        FStringView Source;
        USize Cursor      = 0;
        USize CurrentLine = 1;
    };

} // namespace Lumen

} // namespace LumenEngine