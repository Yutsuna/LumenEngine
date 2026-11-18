/**
 * @file IRHIResource.hpp
 * @brief Declares common resource memory and usage queries.
 */

#pragma once

#include <Memory/RHIResourceLifetime.hpp>
#include <Memory/RHIResourceRequirements.hpp>
#include <Resources/RHIResourceUsage.hpp>
#include <Types/IRHIObject.hpp>
#include <Types/RHIResourceState.hpp>

namespace Lumen
{

/**
 * @class IRHIResource
 * @brief Common base of buffers and textures.
 */
class IRHIResource : public IRHIObject
{
public:

    /** @return Memory requirements computed at creation. */
    [[nodiscard]] virtual const FRHIResourceRequirements &GetRequirements () const noexcept = 0;

    /** @return Legal usage flags. */
    [[nodiscard]] virtual ERHIResourceUsage GetUsage () const noexcept = 0;

    /** @return Initial state assigned at creation. */
    [[nodiscard]] virtual ERHIResourceState GetInitialState () const noexcept = 0;

    /** @return Current GPU lifetime interval. */
    [[nodiscard]] virtual FRHIResourceLifetime GetLifetime () const noexcept = 0;
};
} // namespace Lumen
