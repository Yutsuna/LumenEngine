///
/// @file PlatformTime.cpp
/// @brief Platform-specific time utilities for high-resolution timing and performance measurement.
///

#include <HAL/PlatformTime.hpp>

#include <chrono>
#include <thread>

Lumen::Float64 Lumen::HAL::FPlatformTime::Seconds () noexcept
{
    using FTimePoint                             = std::chrono::steady_clock::time_point;
    static const FTimePoint StartTime            = std::chrono::steady_clock::now();
    const FTimePoint CurrentTime                 = std::chrono::steady_clock::now();
    const std::chrono::duration<Float64> Elapsed = CurrentTime - StartTime;

    return Elapsed.count();
}

Lumen::UInt64 Lumen::HAL::FPlatformTime::NowNanoseconds () noexcept
{
    return static_cast<UInt64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() );
}

void Lumen::HAL::FPlatformTime::Sleep ( const Float64 Seconds ) noexcept
{
    std::this_thread::sleep_for( std::chrono::duration<Float64>( Seconds ) );
}
