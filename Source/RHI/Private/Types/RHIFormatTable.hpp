/**
 * @file RHIFormatTable.hpp
 * @brief Declares the internal canonical format table.
 */
#pragma once

#include <Memory/Span.hpp>
#include <Types/RHIFormatInfo.hpp>

namespace Lumen
{

/** @return Read-only table indexed by ERHIFormat, with Count entries. */
[[nodiscard]] TSpan<const FRHIFormatInfo> GetRHIFormatTable () noexcept;

} // namespace Lumen
