/**
 * @file IRHISampler.hpp
 * @brief Declares an immutable sampler object.
 */

#pragma once

#include <Resources/RHISamplerDesc.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
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
} // namespace Lumen
