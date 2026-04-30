/**
 * @file DLSLScanner.hpp
 * @brief Zero-allocation lexical scanner for DLSL source files.
 *        Relocated from Source/Lumen into the Compiler module.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/String.hpp"

namespace LumenEngine
{

namespace Compiler
{

    // -----------------------------------------------------------------------
    // Token
    // -----------------------------------------------------------------------

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

        /** @brief Construct a token from its components. */
        [[nodiscard]] static FToken Make ( EType InType, FStringView InText, USize InLine ) noexcept;
    };

    // -----------------------------------------------------------------------
    // Scanner
    // -----------------------------------------------------------------------

    /**
     * @class FDLSLScanner
     * @brief Zero-allocation lexical scanner that tokenizes a DLSL source string view.
     *
     * The scanner holds a non-owning view of the source buffer. All returned
     * token text views point into that same buffer, so the caller must ensure
     * the source buffer outlives every token produced.
     */
    class LUMEN_ENGINE_API FDLSLScanner final
    {
    public:

        /**
         * @brief Constructs the scanner over the provided source view.
         * @param InSource Non-owning view of the DLSL source text.
         */
        explicit FDLSLScanner ( FStringView InSource ) noexcept;

    public:

        /**
         * @brief Scan and return the next token from the source.
         * @return The next token; type is EType::EndOfFile when exhausted.
         */
        [[nodiscard]] FToken ScanToken () noexcept;

    private:

        /** @brief Skip whitespace and line comments (// ... \n). */
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

} // namespace Compiler

} // namespace LumenEngine
