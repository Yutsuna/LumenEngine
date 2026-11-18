/**
 * @file IRHISampler.hpp
 * @brief Declares an immutable sampler object.
 */

#pragma once

#include <RHI/Resources/RHISamplerDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHISampler
     * @brief Immutable filtering and addressing state.
     */
    class IRHISampler : public IRHIObject
    {
    public:

        /** @return Creation description. */
        [[nodiscard]] virtual const FRHISamplerDesc &GetDesc () const noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
