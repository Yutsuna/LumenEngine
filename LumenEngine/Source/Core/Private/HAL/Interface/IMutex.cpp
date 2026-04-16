/**
 * @file IMutex.cpp
 * @brief Implementation for mutex types in Lumen Engine.
 */

#include "HAL/Interface/IMutex.hpp"

void LumenEngine::Internal::IMutex::lock () noexcept
{
    Lock();
}

void LumenEngine::Internal::IMutex::unlock () noexcept
{
    Unlock();
}

[[nodiscard]] LumenEngine::Bool LumenEngine::Internal::IMutex::try_lock () noexcept
{
    return TryLock();
}
