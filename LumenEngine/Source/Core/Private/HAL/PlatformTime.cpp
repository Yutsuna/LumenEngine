/**
 * @file PlatformTime.hpp
 * @brief Platform-specific time utilities for high-resolution timing and performance measurement.
 */

#include "HAL/PlatformTime.hpp"

#include <chrono>
#include <thread>

LumenEngine::Float64 LumenEngine::FPlatformTime::Seconds ()
{
    static const auto StartTime                  = std::chrono::steady_clock::now();
    const auto CurrentTime                       = std::chrono::steady_clock::now();
    const std::chrono::duration<Float64> Elapsed = CurrentTime - StartTime;

    return Elapsed.count();
}

void LumenEngine::FPlatformTime::Sleep ( const Float64 Seconds )
{
    std::this_thread::sleep_for( std::chrono::duration<Float64>( Seconds ) );
}
