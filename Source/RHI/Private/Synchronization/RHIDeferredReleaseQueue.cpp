/**
 * @file RHIDeferredReleaseQueue.cpp
 * @brief Implements timeline-gated retirement callbacks.
 */

#include <HAL/LockGuard.hpp>
#include <Synchronization/RHIDeferredReleaseQueue.hpp>
Lumen::FRHIDeferredReleaseQueue::~FRHIDeferredReleaseQueue () noexcept
{
    Shutdown();
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIDeferredReleaseQueue::Enqueue ( const FRHITimelineValue InRequired,
                                                                                    TFunction<void()> InRelease ) noexcept
{
    if ( not InRelease )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidSynchronization( "Release callback is empty" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bShutdown )
    {
        return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::InvalidState, ERHIErrorCondition::Synchronization,
                                                  "Deferred release queue is shut down" ) );
    }
    Entries.push_back( { InRequired, std::move( InRelease ) } );
    return {};
}
Lumen::USize Lumen::FRHIDeferredReleaseQueue::Poll ( const FRHITimelineValue InCompleted ) noexcept
{
    TVector<FEntry> Ready;
    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        TVector<FEntry> Remaining;
        for ( auto &Entry : Entries )
        {
            if ( IsRHICompletionReached( InCompleted, Entry.Required ) )
            {
                Ready.push_back( std::move( Entry ) );
            }
            else
            {
                Remaining.push_back( std::move( Entry ) );
            }
        }
        Entries = std::move( Remaining );
    }
    for ( auto &Entry : Ready )
    {
        Entry.Release();
    }
    return Ready.size();
}
Lumen::USize Lumen::FRHIDeferredReleaseQueue::Drain () noexcept
{
    TVector<FEntry> Ready;
    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        Ready = std::move( Entries );
        Entries.clear();
    }
    for ( auto &Entry : Ready )
    {
        Entry.Release();
    }
    return Ready.size();
}
void Lumen::FRHIDeferredReleaseQueue::Shutdown () noexcept
{
    {
        HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        if ( bShutdown )
        {
            return;
        }
        bShutdown = true;
    }
    Drain();
}
Lumen::USize Lumen::FRHIDeferredReleaseQueue::GetSize () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Entries.size();
}
Lumen::Bool Lumen::FRHIDeferredReleaseQueue::IsShutdown () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return bShutdown;
}
