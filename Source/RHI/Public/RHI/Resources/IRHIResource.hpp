/**
 * @file IRHIResource.hpp
 * @brief Declares common resource memory and usage queries.
 */

#pragma once

#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <RHI/Memory/RHIResourceRequirements.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>
#include <RHI/Types/IRHIObject.hpp>
#include <RHI/Types/RHIResourceState.hpp>

namespace Lumen
{

namespace RHI
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
} // namespace RHI

} // namespace Lumen
