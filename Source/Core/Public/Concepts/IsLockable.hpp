/**
 * @file IsLockable.hpp
 * @brief Concept for any exposing Lock() and Unlock() type.
 */

#pragma once

namespace Lumen
{

namespace Concepts
{

    /**
     * @concept CIsLockable
     * @brief Concept for any exposing Lock() and Unlock() type.
     */
    template <class TLockableType>
    concept CIsLockable = requires( TLockableType &InLockable ) {
        InLockable.Lock();
        InLockable.Unlock();
    };
} // namespace Concepts

} // namespace Lumen
