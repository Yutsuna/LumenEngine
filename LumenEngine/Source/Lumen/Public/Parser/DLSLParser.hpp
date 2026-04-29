/**
 * @file DLSLParser.hpp
 * @brief Zero-allocation AST parser utilizing FLinearAllocator.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "HAL/Memory/LinearAllocator.hpp"

#include "Parser/DLSLScanner.hpp"
#include "Types/DLSLTypes.hpp"

namespace LumenEngine
{

namespace Lumen
{

    /**
     * @class FDLSLParser
     * @brief Parses a stream of DLSL tokens into an abstract syntax tree (AST
     */
    class LUMEN_ENGINE_API FDLSLParser final
    {
    public:

        /**
         * @brief Constructs a new DLSL parser.
         * @param InSource The source code to parse.
         * @param InAllocator The allocator to use for memory allocation.
         */
        FDLSLParser ( FStringView InSource, HAL::FLinearAllocator &InAllocator ) noexcept;

    public:

        /**
         * @brief Parses the source code into an abstract syntax tree.
         * @return The parsed document or an error message.
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

} // namespace Lumen

} // namespace LumenEngine