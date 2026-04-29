/**
 * @file BinarySerializer.hpp
 * @brief Transformation layer from AST to raw, aligned binary blob.
 */

#pragma once

#include "Definitions.hpp"

#include "Container/Expected.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "Maths/Vertex.hpp"

#include "Types/BinaryTypes.hpp"
#include "Types/DLSLTypes.hpp"

namespace LumenEngine
{

namespace Lumen
{

    /**
     * @class FBinarySerializer
     * @brief Transforms DLSL AST into raw, aligned binary blob.
     */
    class LUMEN_ENGINE_API FBinarySerializer final
    {
    public:

        /**
         * @brief Serializes DLSL AST into raw, aligned binary blob.
         * @param InMeshBlock DLSL AST root block of the mesh.
         * @param InMaterialBlock DLSL AST root block of the material.
         * @return A Vector of the serialized binary data otherwise an error message.
         */
        [[nodiscard]] static TExpected<TVector<Byte>, FString> SerializeMesh ( const FDLSLRootBlock *InMeshBlock );

        /**
         * @brief Serializes DLSL AST into raw, aligned binary blob.
         * @param InMaterialBlock DLSL AST root block of the material.
         * @return A Vector of the serialized binary data otherwise an error message.
         */
        [[nodiscard]] static TExpected<TVector<Byte>, FString> SerializeMaterial ( const FDLSLRootBlock *InMaterialBlock );

    private:

        /**
         * @brief Finds a property in the given object node.
         * @param InObjectNode The object node to search in.
         * @param InKey The key of the property to find.
         * @return A pointer to the found property node, or nullptr if not found.
         */
        [[nodiscard]] static FDLSLNode *FindProperty ( const FDLSLNode *InObjectNode, FStringView InKey ) noexcept;

    private:

        /**
         * @brief Extracts vertex data from a DLSL node.
         * @param InVerticesListNode The node containing the vertex data.
         * @return A vector of vertices on success, otherwise an error message.
         */
        [[nodiscard]] static TExpected<TVector<Maths::FVertex>, FString> ExtractVertices ( const FDLSLNode *InVerticesListNode ) noexcept;

        /**
         * @brief Extracts index data from a DLSL node.
         * @param InIndicesListNode The node containing the index data.
         * @return A vector of indices on success, otherwise an error message.
         */
        [[nodiscard]] static TExpected<TVector<UInt32>, FString> ExtractIndices ( const FDLSLNode *InIndicesListNode ) noexcept;

    private:

        static void ExtractMeshConfig ( const FDLSLNode *InConfigNode, FLumenBinaryMeshHeader &OutHeader ) noexcept;
        static void ExtractRenderState ( const FDLSLNode *InStateNode, FLumenBinaryMaterialHeader &OutHeader ) noexcept;
    };

} // namespace Lumen

} // namespace LumenEngine