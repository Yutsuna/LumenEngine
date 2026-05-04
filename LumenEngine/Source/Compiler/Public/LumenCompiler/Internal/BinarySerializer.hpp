/**
 * @file BinarySerializer.hpp
 * @brief Transforms a DLSL AST into a raw, aligned .lumenbin binary blob.
 *        Relocated from Source/Lumen into the Compiler module.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "LumenCompiler/Internal/DLSLTypes.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include "Maths/Vertex.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @class FBinarySerializer
     * @brief Transforms a DLSL AST root block into a raw, aligned binary blob
     *        conforming to the .lumenbin format.
     */
    class LUMEN_ENGINE_API FBinarySerializer final
    {
    public:

        /**
         * @brief Serialise a @Mesh root block into a binary blob.
         * @param InMeshBlock  Pointer to the parsed root block (must be type "Mesh").
         * @return Raw byte vector on success, or an error string on failure.
         */
        [[nodiscard]] static TExpected<TVector<Byte>, FString> SerializeMesh ( const FDLSLRootBlock *InMeshBlock );

        /**
         * @brief Serialise a @Material root block into a binary blob.
         * @param InMaterialBlock  Pointer to the parsed root block (must be type "Material").
         * @return Raw byte vector on success, or an error string on failure.
         */
        [[nodiscard]] static TExpected<TVector<Byte>, FString> SerializeMaterial ( const FDLSLRootBlock *InMaterialBlock );

    private:

        /**
         * @brief Find a named property inside an object node.
         * @param InObjectNode  An FDLSLNode of type Object.
         * @param InKey         Property name to search for.
         * @return Pointer to the found value node, or nullptr if absent.
         */
        [[nodiscard]] static const FDLSLNode *FindProperty ( const FDLSLNode *InObjectNode, FStringView InKey ) noexcept;

    private:

        /**
         * @brief Extract all vertex data from a List node into a typed vector.
         * @param InVerticesListNode  An FDLSLNode of type List whose elements are Object nodes.
         * @return Vector of FVertex on success, error string on failure.
         */
        [[nodiscard]] static TExpected<TVector<Maths::FVertex>, FString> ExtractVertices ( const FDLSLNode *InVerticesListNode );

        /**
         * @brief Extract all index data from a List node.
         * @param InIndicesListNode  An FDLSLNode of type List whose elements are Number nodes.
         * @return Vector of UInt32 indices on success, error string on failure.
         */
        [[nodiscard]] static TExpected<TVector<UInt32>, FString> ExtractIndices ( const FDLSLNode *InIndicesListNode );

    private:

        /**
         * @brief Populate mesh configuration fields from the @Config object node.
         * @param InConfigNode  Object node containing Topology, CullMode, WindingOrder.
         * @param OutHeader     Header to populate.
         */
        static void ExtractMeshConfig ( const FDLSLNode *InConfigNode, FLumenBinaryMeshHeader &OutHeader ) noexcept;

        /**
         * @brief Populate material render-state fields from the @RenderState object node.
         * @param InStateNode  Object node containing BlendMode, DepthTest, DepthWrite, etc.
         * @param OutHeader    Header to populate.
         */
        static void ExtractRenderState ( const FDLSLNode *InStateNode, FLumenBinaryMaterialHeader &OutHeader ) noexcept;
    };

} // namespace Compiler

} // namespace LumenEngine
