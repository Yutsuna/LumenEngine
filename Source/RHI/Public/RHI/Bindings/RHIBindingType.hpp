/**
 * @file RHIBindingType.hpp
 * @brief Defines descriptor binding types.
 */

#pragma once
#include <Core/CoreTypes.hpp>
namespace Lumen
{

namespace RHI
{
    /**
     * @enum ERHIBindingType
     * @brief Portable descriptor classes.
     */
    enum class ERHIBindingType : UInt8
    {
        ConstantBuffer,
        ReadOnlyBuffer,
        ReadWriteBuffer,
        Texture,
        StorageTexture,
        Sampler,
        CombinedTextureSampler
    };
} // namespace RHI

} // namespace Lumen
