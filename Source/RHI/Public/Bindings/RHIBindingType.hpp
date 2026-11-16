/**
 * @file RHIBindingType.hpp
 * @brief Defines descriptor binding types.
 */

#pragma once
#include <Core/CoreTypes.hpp>
namespace Lumen
{
/** @enum ERHIBindingType @brief Portable descriptor classes. */
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
} // namespace Lumen
