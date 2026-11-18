/**
 * @file IRHIHeap.hpp
 * @brief Declares the backend heap SPI.
 */

#pragma once

#include <Memory/RHIHeapDesc.hpp>
#include <Memory/Span.hpp>
#include <Types/IRHIObject.hpp>

namespace Lumen
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
} // namespace Lumen
