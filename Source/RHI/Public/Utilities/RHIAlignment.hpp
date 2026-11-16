/**
 * @file RHIAlignment.hpp
 * @brief Declares checked power-of-two alignment helpers.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Templates/Optional.hpp>
#include <type_traits>

namespace Lumen
{

/** @return True for a nonzero power of two. */
template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
[[nodiscard]] constexpr Bool RHIIsPowerOfTwo ( ValueType InValue ) noexcept;

/** @return InValue rounded up to InAlignment, or empty on invalid alignment/overflow. */
template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
[[nodiscard]] constexpr TOptional<ValueType> RHIAlignUp ( ValueType InValue, ValueType InAlignment ) noexcept;

/** @return InValue rounded down, or empty for an invalid alignment. */
template <typename ValueType>
    requires std::is_unsigned_v<ValueType>
[[nodiscard]] constexpr TOptional<ValueType> RHIAlignDown ( ValueType InValue, ValueType InAlignment ) noexcept;

/** @return True when two half-open ranges overlap; invalid arithmetic returns false. */
[[nodiscard]] constexpr Bool
RHIRangesOverlap ( UInt64 InFirstOffset, UInt64 InFirstSize, UInt64 InSecondOffset, UInt64 InSecondSize ) noexcept;

} // namespace Lumen

#include "Inline/RHIAlignment.inl"
