/**
 * @file RHITextureViewDesc.hpp
 * @brief Defines a typed view of a texture.
 */
#pragma once
#include <Memory/SharedPtr.hpp>
#include <RHIExport.hpp>
#include <Resources/RHISubresourceRange.hpp>
#include <Types/RHIError.hpp>
#include <Types/RHIFormat.hpp>
namespace Lumen
{
class IRHITexture;
/** @enum ERHITextureViewType @brief Logical view dimensions. */
enum class ERHITextureViewType : UInt8
{
    Texture1D,
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray

};
/**
 * @struct FRHITextureViewDesc
 * @brief Parent texture, optional format override, and subresource range.
 */
struct RHI_EXPORT FRHITextureViewDesc
{
    TSharedPtr<IRHITexture> Texture;
    ERHITextureViewType Type = ERHITextureViewType::Texture2D;
    ERHIFormat Format        = ERHIFormat::Unknown;
    FRHISubresourceRange Range;
    /** @return Success when the parent and range are valid and the format is compatible. */
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
} // namespace Lumen
