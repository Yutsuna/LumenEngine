/**
 * @file AssetDeserializer.hpp
 * @brief Utility for deserializing compiled .lumenbin assets.
 */

#pragma once

#include "LumenCompiler/LumenCompilerTypes.hpp"
#include "Maths/Vertex.hpp"

namespace LumenEngine
{

namespace Compiler
{

    /**
     * @struct FDeserializedMesh
     * @brief Result of deserializing a mesh asset.
     */
    struct FDeserializedMesh
    {
        TVector<Maths::FVertex> Vertices;
        TVector<UInt32> Indices;
        FLumenBinaryMeshHeader Header;
    };

    /**
     * @class FAssetDeserializer
     * @brief Helper class to convert raw binary blobs back into engine-usable structures.
     */
    class LUMEN_ENGINE_API FAssetDeserializer final
    {
    public:

        /**
         * @brief Deserializes a binary blob into a mesh structure.
         * @param InBlob The raw binary blob from FCompiledLumenAsset.
         * @return A deserialized mesh, or std::nullopt if the blob is malformed.
         */
        [[nodiscard]] static TOptional<FDeserializedMesh> DeserializeMesh ( const TVector<Byte> &InBlob ) noexcept;

        /**
         * @brief Deserializes a binary blob into a material header.
         * @param InBlob The raw binary blob from FCompiledLumenAsset.
         * @return A deserialized material header, or std::nullopt if the blob is malformed.
         */
        [[nodiscard]] static TOptional<FLumenBinaryMaterialHeader> DeserializeMaterial ( const TVector<Byte> &InBlob ) noexcept;
    };

} // namespace Compiler

} // namespace LumenEngine
