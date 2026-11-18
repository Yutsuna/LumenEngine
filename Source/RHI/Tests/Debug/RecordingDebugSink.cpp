/**
 * @file RecordingDebugSink.cpp
 * @brief Implements ordered thread-safe diagnostic recording.
 */

#include <Debug/RecordingDebugSink.hpp>
#include <HAL/LockGuard.hpp>

void Lumen::RHI::Tests::FRecordingDebugSink::Emit ( const FRHIDebugMessage &InMessage ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Messages.push_back( InMessage );
}

Lumen::TVector<Lumen::RHI::FRHIDebugMessage> Lumen::RHI::Tests::FRecordingDebugSink::GetMessages () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Messages;
}

Lumen::Bool Lumen::RHI::Tests::FRecordingDebugSink::ContainsCode ( const UInt32 InCode ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( const auto &Message : Messages )
    {
        if ( Message.Code == InCode )
        {
            return true;
        }
    }
    return false;
}

Lumen::USize Lumen::RHI::Tests::FRecordingDebugSink::GetCount ( const ERHIDebugSeverity InMinimumSeverity ) const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    USize Count = 0U;
    for ( const auto &Message : Messages )
    {
        if ( Message.IsAtLeast( InMinimumSeverity ) )
        {
            ++Count;
        }
    }
    return Count;
}

void Lumen::RHI::Tests::FRecordingDebugSink::Clear () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Messages.clear();
}
