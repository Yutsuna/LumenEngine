/**
 * @file Hash.hpp
 * @brief Computes a hash value for a given type.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

/**
 * @namespace FHash
 * @brief Hash functions helpers.
 */
namespace FHash
{

    /**
     * @brief Computes a stable hash value for a given type.
     * @tparam TObjectType The type to compute the hash for.
     */
    template <class TObjectType> consteval UInt64 TypeHash () noexcept;

} // namespace FHash

} // namespace Lumen

#include "Inline/Hash.inl"
