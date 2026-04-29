/**
 * @file BinaryTypes.hpp
 * @brief Memory-mapped binary structures for Lumen assets.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"
#include "Version.hpp"

namespace LumenEngine
{

namespace Lumen
{

    namespace EAssetType
    {

        enum Type : UInt32
        {
            Mesh     = 1,
            Material = 2,
        };

        constexpr const AnsiChar *ToString ( Type AssetType ) noexcept;

    } // namespace EAssetType

    /**
     * @struct FLumenBinaryHeader
     * @brief 16-byte aligned header for all .lumenbin files.
     */
    struct alignas( 16 ) FLumenBinaryHeader
    {
        UInt32 Magic   = LUMEN_ASSET_CACHE_MAGIC_NUMBER; //<< LMAS = Lumen Asset Cache
        UInt32 Version = Version::Packed;                //<< 1
        EAssetType::Type AssetType;                      //<< 1 = Mesh, 2 = Material...
        UInt32 PayloadSize;                              //<< Size of the data following the header
    };

    /**
     * @struct FLumenBinaryMeshHeader
     * @brief 32-byte header for compiled meshes.
     */
    struct alignas( 16 ) FLumenBinaryMeshHeader
    {
        UInt32 VertexCount;
        UInt32 IndexCount;
        UInt32 Topology;     //<< Maps to VkPrimitiveTopology
        UInt32 CullMode;     //<< Maps to VkCullModeFlags
        UInt32 WindingOrder; //<< Maps to VkFrontFace
        UInt32 Pad[3];
    };

    /**
     * @struct FLumenBinaryMaterialHeader
     * @brief 32-byte header for compiled materials.
     */
    struct alignas( 16 ) FLumenBinaryMaterialHeader
    {
        UInt32 BlendMode;
        UInt32 DepthTest;
        UInt32 DepthWrite;
        UInt32 CullMode;
        UInt32 WireFrame;
        UInt32 PropertyCount;
        UInt32 Pad[2];
    };

} // namespace Lumen

} // namespace LumenEngine

#include "Inline/BinaryTypes.inl"