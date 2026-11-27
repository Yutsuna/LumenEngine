/**
 * @file RHICheckedMath.hpp
 * @brief Declares overflow-checked integer arithmetic.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Templates/Optional.hpp>

#include <Concepts/IsIntegral.hpp>

#include <type_traits>

namespace Lumen
{

namespace RHI
{

    /** @return Sum of two unsigned values, or empty on overflow. */
    template <typename ValueType>
        requires std::is_unsigned_v<ValueType>
    [[nodiscard]] constexpr TOptional<ValueType> RHICheckedAdd ( ValueType InLeft, ValueType InRight ) noexcept;

    /** @return Product of two unsigned values, or empty on overflow. */
    template <typename ValueType>
        requires std::is_unsigned_v<ValueType>
    [[nodiscard]] constexpr TOptional<ValueType> RHICheckedMultiply ( ValueType InLeft, ValueType InRight ) noexcept;

    /** @return Ceiling division for a nonzero divisor, otherwise empty. */
    template <typename ValueType>
        requires std::is_unsigned_v<ValueType>
    [[nodiscard]] constexpr TOptional<ValueType> RHICheckedDivideCeil ( ValueType InDividend, ValueType InDivisor ) noexcept;

    /** @return InValue converted to the destination type when representable. */
    template <typename DestinationType, typename SourceType>
        requires Concepts::CIsIntegral<DestinationType> and Concepts::CIsIntegral<SourceType>
    [[nodiscard]] constexpr TOptional<DestinationType> RHICheckedCast ( SourceType InValue ) noexcept;

} // namespace RHI

} // namespace Lumen

#include "Inline/RHICheckedMath.inl"
