///
/// @file IMutex.hpp
/// @brief Interface for mutex types in Lumen Engine.
///

#pragma once

#include <Core/CoreTypes.hpp>
#include <CoreExport.hpp>

namespace Lumen
{

namespace Internal
{

    ///
    /// @interface IMutex
    /// @brief Interface for mutex types in Lumen Engine.
    ///
    class CORE_EXPORT IMutex
    {
    public:

        virtual ~IMutex () noexcept = default;

    public:

        /// @brief Lock the mutex
        virtual void Lock () noexcept = 0;

        /// @brief Unlock the mutex
        virtual void Unlock () noexcept = 0;

    public:

        /// @return True if successfully lock, otherwise false
        [[nodiscard]] virtual Bool TryLock () noexcept = 0;

        /// @brief True if the mutex is locked, otherwise false
        [[nodiscard]] virtual Bool IsLocked () const noexcept          = 0;
        [[nodiscard]] virtual explicit operator Bool () const noexcept = 0;

    public:

        /// @brief Lock for API
        void lock () noexcept;
        /// @brief Unlock for API
        void unlock () noexcept;
        /// @brief Try lock for API
        [[nodiscard]] Bool try_lock () noexcept;
    };

} // namespace Internal

} // namespace Lumen