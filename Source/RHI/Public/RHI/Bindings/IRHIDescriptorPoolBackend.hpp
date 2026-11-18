/**
 * @file IRHIDescriptorPoolBackend.hpp
 * @brief Declares the native descriptor-pool SPI.
 */

#pragma once
#include <Core/CoreTypes.hpp>
#include <RHI/Types/RHIError.hpp>
namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIDescriptorPoolBackend
     * @brief Minimal native pool operations used by the common allocator.
     */
    class IRHIDescriptorPoolBackend
    {
    public:

        virtual ~IRHIDescriptorPoolBackend () noexcept = default;
        /** @brief Creates a native pool with InCapacity entries and returns its backend token. */
        [[nodiscard]] virtual TExpected<UInt64, FRHIError> CreatePool ( UInt32 InCapacity ) noexcept = 0;
        /** @brief Releases a native pool token after GPU completion. */
        virtual void ReleasePool ( UInt64 InPool ) noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
