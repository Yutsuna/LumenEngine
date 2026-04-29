/**
 * @file DLSLTypes.hpp
 * @brief AST Node definitions for the DLSL parser.
 */

#pragma once

#include "Container/String.hpp"
#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace Lumen
{

    namespace EDLSLNodeType
    {

        enum Type : UInt8
        {
            String,
            Number,
            Vector,
            Boolean,
            Identifier,
            List,
            Object
        };

        static constexpr const AnsiChar *ToString ( Type InType ) noexcept;

    } // namespace EDLSLNodeType

    struct FDLSLNode;

    struct FDLSLProperty
    {
        FStringView Key;
        FDLSLNode *Value    = nullptr;
        FDLSLProperty *Next = nullptr;
    };

    struct FDLSLNode
    {
        EDLSLNodeType::Type Type;

        union
        {
            struct
            {
                const char *Data;
                USize Size;
            } StringRaw;

            Float64 NumberValue;

            struct
            {
                Float32 Data[4];
                UInt8 Count;
            } VectorValue;

            Bool BooleanValue;

            struct
            {
                const char *Data;
                USize Size;

            } IdentifierRaw;

            struct
            {
                FDLSLNode *Head;
                FDLSLNode *Tail;
                USize Count;
            } ListValue;

            struct
            {
                FDLSLProperty *Head;
                FDLSLProperty *Tail;
                USize Count;
            } ObjectValue;
        };

        FDLSLNode *Next = nullptr;

        [[nodiscard]] FStringView GetString () const noexcept;
    };

    struct FDLSLRootBlock
    {
        FStringView BlockType;
        FStringView Name;
        FDLSLNode *Body      = nullptr;
        FDLSLRootBlock *Next = nullptr;
    };

    struct FDLSLDocument
    {
        FDLSLRootBlock *FirstBlock = nullptr;
    };

} // namespace Lumen

} // namespace LumenEngine

#include "Inline/DLSLTypes.inl"
