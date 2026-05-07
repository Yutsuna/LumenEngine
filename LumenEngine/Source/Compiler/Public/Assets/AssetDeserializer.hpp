/**
 * @file AssetDeserializer.hpp
 * @brief Utility for deserializing compiled .lumenbin assets.
 */

#pragma once

#include "Container/Expected.hpp"
#include "Container/Span.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"
#include "Maths/Vertex.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @struct FDeserializedMesh
     * @brief Result of deserializing a mesh asset with ownership.
     */
    struct FDeserializedMesh
    {
        TVector<Maths::FVertex> Vertices;
        TVector<UInt32> Indices;
        FLumenBinaryMeshHeader Header;
    };

    /**
     * @struct FMeshView
     * @brief Zero-copy view of a deserialized mesh asset.
     */
    struct FMeshView
    {
        TSpan<const Maths::FVertex> Vertices;
        TSpan<const UInt32> Indices;
        FLumenBinaryMeshHeader Header;
    };

    /**
     * @class FAssetDeserializer
     * @brief Helper class to convert raw binary blobs back into engine-usable structures.
     *        Optimized for C++23 zero-copy operations.
     */
    class LUMEN_ENGINE_API FAssetDeserializer final
    {
    public:

        /**
         * @brief Deserializes a binary blob into a mesh structure (Copy version).
         * @param InBlob The raw binary blob.
         * @return A deserialized mesh, or an error code on failure.
         */
        [[nodiscard]] static TExpected<FDeserializedMesh, ELumenCompilerError::Type> DeserializeMesh ( TSpan<const Byte> InBlob ) noexcept;

        /**
         * @brief Deserializes a binary blob into a mesh view (Zero-copy version).
         * @param InBlob The raw binary blob.
         * @return A mesh view pointing into the provided blob, or an error code on failure.
         */
        [[nodiscard]] static TExpected<FMeshView, ELumenCompilerError::Type> DeserializeMeshView ( TSpan<const Byte> InBlob ) noexcept;

        /**
         * @brief Deserializes a binary blob into a material header.
         * @param InBlob The raw binary blob.
         * @return A deserialized material header, or an error code on failure.
         */
        [[nodiscard]] static TExpected<FLumenBinaryMaterialHeader, ELumenCompilerError::Type> DeserializeMaterial ( TSpan<const Byte> InBlob ) noexcept;
    };

} // namespace Compiler

} // namespace LumenEngine
