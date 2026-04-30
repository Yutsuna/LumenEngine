/**
 * @file DLSLParser.hpp
 * @brief Zero-allocation AST parser for DLSL source, using FLinearAllocator.
 *        Relocated from Source/Lumen into the Compiler module.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "Container/String.hpp"
#include "HAL/Memory/LinearAllocator.hpp"

#include "LumenCompiler/Internal/DLSLScanner.hpp"
#include "LumenCompiler/Internal/DLSLTypes.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class FDLSLParser
     * @brief Parses a stream of DLSL tokens into an Abstract Syntax Tree.
     *
     * All AST nodes are allocated from the provided FLinearAllocator, which
     * means the entire AST can be freed in O(1) by resetting the allocator.
     * The parser does not own any memory.
     *
     * The source string view passed to the scanner must remain valid for the
     * entire lifetime of the resulting AST, because token text views point
     * directly into the original buffer.
     */
    class LUMEN_ENGINE_API FDLSLParser final
    {
    public:

        /**
         * @brief Constructs the parser.
         * @param InSource  The DLSL source text to parse (non-owning view).
         * @param InAllocator  Linear allocator that backs all AST node allocations.
         */
        FDLSLParser ( FStringView InSource, HAL::FLinearAllocator &InAllocator ) noexcept;

    public:

        /**
         * @brief Parse the full source into an FDLSLDocument AST.
         * @return The parsed document on success, or an error message on failure.
         */
        [[nodiscard]] TExpected<FDLSLDocument *, FString> Parse ();

    private:

        [[nodiscard]] TExpected<FDLSLRootBlock *, FString> ParseRootBlock ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseNode ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseObject ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseList ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseVector ();
        [[nodiscard]] TExpected<FDLSLProperty *, FString> ParseProperty ();

        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseStringNode ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseNumberNode ();
        [[nodiscard]] TExpected<FDLSLNode *, FString> ParseIdentifierOrBooleanNode ();

    private:

        FToken Advance () noexcept;
        [[nodiscard]] FToken Peek () const noexcept;
        Bool Match ( FToken::EType InType ) noexcept;

        [[nodiscard]] TExpected<FToken, FString> Consume ( FToken::EType InType, FStringView InErrorMessage ) noexcept;

        template <typename Type, typename... Args> [[nodiscard]] TExpected<Type *, FString> AllocateNode ( Args &&...InArgs );

    private:

        FDLSLScanner Scanner;
        HAL::FLinearAllocator &Allocator;

        FToken CurrentToken;
        FToken PreviousToken;
    };

} // namespace Compiler

} // namespace LumenEngine
