/**
 * @file Thread.cpp
 * @brief Implementation of FThread.
 */

#include <Thread/Thread.hpp>

#include <utility>

Lumen::FThread::FThread ( FThread &&InOther ) noexcept
    : StopSource( std::move( InOther.StopSource ) ), Thread( std::move( InOther.Thread ) )
{
}

Lumen::FThread::~FThread () noexcept
{
    static_cast<void>( RequestStop() );
    Join();
}

Lumen::FThread &Lumen::FThread::operator=( FThread &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        static_cast<void>( RequestStop() );
        Join();
        StopSource = std::move( InOther.StopSource );
        Thread     = std::move( InOther.Thread );
    }
    return *this;
}

void Lumen::FThread::Join () noexcept
{
    Thread.Join();

    if ( not Thread.IsJoinable() )
    {
        StopSource = FStopSource( nullptr );
    }
}

void Lumen::FThread::Detach () noexcept
{
    Thread.Detach();

    if ( not Thread.IsJoinable() )
    {
        StopSource = FStopSource( nullptr );
    }
}

void Lumen::FThread::Swap ( FThread &InOther ) noexcept
{
    StopSource.Swap( InOther.StopSource );
    Thread.Swap( InOther.Thread );
}

Lumen::Bool Lumen::FThread::IsJoinable () const noexcept
{
    return Thread.IsJoinable();
}

Lumen::Bool Lumen::FThread::RequestStop () noexcept
{
    return StopSource.RequestStop();
}

Lumen::FStopSource Lumen::FThread::GetStopSource () const noexcept
{
    return StopSource;
}

Lumen::FStopToken Lumen::FThread::GetStopToken () const noexcept
{
    return StopSource.GetToken();
}

Lumen::FThread::FId Lumen::FThread::GetId () const noexcept
{
    return Thread.GetId();
}

Lumen::FThread::FNativeHandle Lumen::FThread::GetNativeHandle () const noexcept
{
    return Thread.GetNativeHandle();
}

Lumen::FThread::FId Lumen::FThread::GetCurrentId () noexcept
{
    return HAL::FPlatformThread::GetCurrentId();
}

Lumen::UInt32 Lumen::FThread::HardwareConcurrency () noexcept
{
    return HAL::FPlatformThread::HardwareConcurrency();
}

void Lumen::FThread::Yield () noexcept
{
    HAL::FPlatformThread::Yield();
}
