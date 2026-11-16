/**
 * @file RHIHash.hpp
 * @brief Defines deterministic content hashing for RHI values.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <Memory/Span.hpp>
#include <RHIExport.hpp>
#include <type_traits>

namespace Lumen
{

/** @return Stable FNV-1a hash of a byte sequence. */
[[nodiscard]] RHI_EXPORT UInt64 RHIHashBytes ( TSpan<const Byte> InBytes ) noexcept;

/** @return A stable combination of two previously computed hashes. */
[[nodiscard]] RHI_EXPORT UInt64 RHIHashCombine ( UInt64 InSeed, UInt64 InValue ) noexcept;

/** @brief Hashes the object representation of a trivially copyable value. */
template <typename ValueType>
    requires std::is_trivially_copyable_v<ValueType>
[[nodiscard]] UInt64 RHIHashValue ( const ValueType &InValue ) noexcept;

/**
 * @struct TRHIValueHasher
 * @brief Hash functor suitable for TMap.
 */
template <typename ValueType> struct TRHIValueHasher
{
    /** @return Content hash of InValue. */
    [[nodiscard]] USize operator()( const ValueType &InValue ) const noexcept;
};

} // namespace Lumen

#include "Inline/RHIHash.inl"
