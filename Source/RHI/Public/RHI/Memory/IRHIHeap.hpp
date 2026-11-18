/**
 * @file IRHIHeap.hpp
 * @brief Declares the backend heap SPI.
 */

#pragma once

#include <Memory/Span.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Types/IRHIObject.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIHeap
     * @brief Native or simulated memory owned by a device.
     */
    class IRHIHeap : public IRHIObject
    {
    public:

        /** @return Creation description. */
        [[nodiscard]] virtual const FRHIHeapDesc &GetDesc () const noexcept = 0;
        /** @brief Maps an aligned heap range for CPU access. */
        [[nodiscard]] virtual TExpected<TSpan<Byte>, FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept = 0;
        /** @brief Unmaps a previously mapped range. */
        virtual void Unmap () noexcept = 0;
    };
} // namespace RHI

} // namespace Lumen
