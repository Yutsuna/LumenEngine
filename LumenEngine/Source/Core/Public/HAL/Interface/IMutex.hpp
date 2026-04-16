/**
 * @file IMutex.hpp
 * @brief Interface for mutex types in Lumen Engine.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

namespace LumenEngine
{

namespace Internal
{

    /**
     * @interface IMutex
     * @brief Interface for mutex types in Lumen Engine.
     */
    class LUMEN_ENGINE_API IMutex
    {
    public:

        virtual ~IMutex () noexcept = default;

    public:

        virtual void Lock () noexcept   = 0;
        virtual void Unlock () noexcept = 0;

    public:

        [[nodiscard]] virtual Bool TryLock () noexcept                 = 0;
        [[nodiscard]] virtual Bool IsLocked () const noexcept          = 0;
        [[nodiscard]] virtual explicit operator Bool () const noexcept = 0;

    public:

        // NOLINTBEGIN(readability-identifier-naming)
        void lock () noexcept;
        void unlock () noexcept;
        [[nodiscard]] Bool try_lock () noexcept;
        // NOLINTEND(readability-identifier-naming)
    };

} // namespace Internal

} // namespace LumenEngine