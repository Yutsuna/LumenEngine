///
/// @file IMutex.cpp
/// @brief Implementation for mutex types in Lumen Engine.
///

#include <HAL/Interface/IMutex.hpp>

void Lumen::Internal::IMutex::lock() noexcept
{
    Lock();
}

void Lumen::Internal::IMutex::unlock() noexcept
{
    Unlock();
}

[[nodiscard]] Lumen::Bool Lumen::Internal::IMutex::try_lock() noexcept
{
    return TryLock();
}