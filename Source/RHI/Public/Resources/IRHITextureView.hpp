/**
 * @file IRHITextureView.hpp
 * @brief Declares a view retaining its parent texture.
 */
#pragma once
#include <Resources/IRHITexture.hpp>
#include <Resources/RHITextureViewDesc.hpp>
namespace Lumen
{

/**
 * @class IRHITextureView
 * @brief Typed view of a texture subresource range.
 */
class IRHITextureView : public IRHIObject
{
public:

    /** @return View creation description. */
    [[nodiscard]] virtual const FRHITextureViewDesc &GetDesc () const noexcept = 0;
    /** @return Strong reference to the parent texture. */
    [[nodiscard]] virtual TSharedPtr<IRHITexture> GetTexture () const noexcept = 0;
};
} // namespace Lumen
