/**
 * @file IRHITexture.hpp
 * @brief Declares a portable texture resource.
 */

#pragma once

#include <Resources/IRHIResource.hpp>
#include <Resources/RHITextureDesc.hpp>

namespace Lumen
{

/**
 * @class IRHITexture
 * @brief Multi-dimensional resource with subresources.
 */
class IRHITexture : public IRHIResource
{
public:

    /** @return Creation description. */
    [[nodiscard]] virtual const FRHITextureDesc &GetDesc () const noexcept = 0;
};
} // namespace Lumen
