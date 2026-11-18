/**
 * @file RHIFormatTable.hpp
 * @brief Declares the internal canonical format table.
 */
#pragma once

#include <Memory/Span.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>

namespace Lumen
{

namespace RHI
{

    /** @return Read-only table indexed by ERHIFormat, with Count entries. */
    [[nodiscard]] TSpan<const FRHIFormatInfo> GetRHIFormatTable () noexcept;

} // namespace RHI

} // namespace Lumen
