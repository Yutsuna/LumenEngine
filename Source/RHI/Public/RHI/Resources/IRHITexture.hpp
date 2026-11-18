/**
 * @file IRHITexture.hpp
 * @brief Declares a portable texture resource.
 */

#pragma once

#include <RHI/Resources/IRHIResource.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>

namespace Lumen
{

namespace RHI
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
} // namespace RHI

} // namespace Lumen
