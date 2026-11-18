/**
 * @file IRHIBuffer.hpp
 * @brief Declares a portable buffer resource.
 */

#pragma once

#include <Memory/Span.hpp>
#include <Resources/IRHIResource.hpp>
#include <Resources/RHIBufferDesc.hpp>

namespace Lumen
{

/**
 * @class IRHIBuffer
 * @brief Linear resource with optional CPU mapping.
 */
class IRHIBuffer : public IRHIResource
{
public:

    /** @return Creation description. */
    [[nodiscard]] virtual const FRHIBufferDesc &GetDesc () const noexcept = 0;

    /** @brief Maps a CPU-visible range. */
    [[nodiscard]] virtual TExpected<TSpan<Byte>, FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept = 0;

    /** @brief Unmaps the current mapping. */
    virtual void Unmap () noexcept = 0;
};
} // namespace Lumen
