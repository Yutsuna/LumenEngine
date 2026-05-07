/**
 * @file DLSLTypes.hpp
 * @brief AST Node definitions for the DLSL parser within the Compiler module.
 */

#pragma once

#include "Container/String.hpp"
#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @enum EDLSLNodeType
     * @brief Enumeration of possible DLSL node types, used to identify the type of value stored in a node.
     */
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

        [[nodiscard]] static constexpr const AnsiChar *ToString ( Type InType ) noexcept;

    } // namespace EDLSLNodeType

    struct FDLSLNode;

    /**
     * @struct FDLSLProperty
     * @brief Represents a key-value pair in an object node
     */
    struct FDLSLProperty
    {
        FStringView Key;
        FDLSLNode *TypeNode = nullptr;
        FDLSLNode *Value    = nullptr;
        FDLSLProperty *Next = nullptr;
    };

#if defined( LUMEN_ENGINE_COMPILER_CLANG )
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wc11-extensions"
    #pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
    #pragma clang diagnostic ignored "-Wnested-anon-types"
#elif defined( LUMEN_ENGINE_COMPILER_GCC )
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"
#endif

    /**
     * @struct FDLSLNode
     * @brief Represents a node in the DLSL AST
     */
    struct FDLSLNode
    {
        EDLSLNodeType::Type Type;

        union
        {

            /** @brief Raw string data for String nodes. The actual string view can be obtained using GetString(). */
            struct
            {
                const char *Data;
                USize Size;
            } StringRaw;

            Float64 NumberValue;

            /** @brief Vector data for Vector nodes. */
            struct
            {
                Float32 Data[4];
                UInt8 Count;
            } VectorValue;

            Bool BooleanValue;

            /** @brief Raw string data for Identifier nodes. The actual string view can be obtained using GetString(). */
            struct
            {
                const char *Data;
                USize Size;
            } IdentifierRaw;

            /** @brief List of nodes for List nodes. The Count field indicates how many nodes are in the list. */
            struct
            {
                FDLSLNode *Head;
                FDLSLNode *Tail;
                USize Count;
            } ListValue;

            /** @brief List of properties for Object nodes. The Count field indicates how many properties are in the object. */
            struct
            {
                FDLSLProperty *Head;
                FDLSLProperty *Tail;
                USize Count;
            } ObjectValue;
        };

        FDLSLNode *Next = nullptr;

        /** @brief Returns the string view for String or Identifier nodes. */
        [[nodiscard]] FStringView GetString () const noexcept;
    };

#if defined( LUMEN_ENGINE_COMPILER_CLANG )
    #pragma clang diagnostic pop
#elif defined( LUMEN_ENGINE_COMPILER_GCC )
    #pragma GCC diagnostic pop
#endif

    /**
     * @struct FDLSLRootBlock
     * @brief Root block (@BlockType "Name" { ... })
     */
    struct FDLSLRootBlock
    {
        FStringView BlockType;
        FStringView Name;
        FDLSLNode *Body      = nullptr;
        FDLSLRootBlock *Next = nullptr;
    };

    /**
     * @struct FDLSLDocument
     * @brief Represents the entire DLSL document, containing a linked list of root blocks.
     */
    struct FDLSLDocument
    {
        FDLSLRootBlock *FirstBlock = nullptr;
    };

} // namespace Compiler

} // namespace LumenEngine

#include "Inline/DLSLTypes.inl"
