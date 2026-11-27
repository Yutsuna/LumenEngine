/**
 * @file RHIDeferredReleaseQueue.cpp
 * @brief Implements timeline-gated retirement callbacks.
 */

#include <HAL/LockGuard.hpp>
#include <RHI/Synchronization/RHIDeferredReleaseQueue.hpp>
Lumen::RHI::FRHIDeferredReleaseQueue::~FRHIDeferredReleaseQueue () noexcept
{
    Shutdown();
}
Lumen::RHI::FRHIExpected Lumen::RHI::FRHIDeferredReleaseQueue::Enqueue ( const FRHITimelineValue InRequired,
                                                                         TFunction<void()> InRelease ) noexcept
{
    if ( not InRelease )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidSynchronization( "Release callback is empty" ) );
    }
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    if ( bShutdown )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidState( ERHIErrorCondition::Synchronization, "Deferred release queue is shut down" ) );
    }
    Entries.push_back( { InRequired, std::move( InRelease ) } );
    return {};
}
Lumen::USize Lumen::RHI::FRHIDeferredReleaseQueue::Poll ( const FRHITimelineValue InCompleted ) noexcept
{
    TVector<FEntry> Ready;
    {
        const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
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
Lumen::USize Lumen::RHI::FRHIDeferredReleaseQueue::Drain () noexcept
{
    TVector<FEntry> Ready;
    {
        const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        Ready = std::move( Entries );
        Entries.clear();
    }
    for ( auto &Entry : Ready )
    {
        Entry.Release();
    }
    return Ready.size();
}
void Lumen::RHI::FRHIDeferredReleaseQueue::Shutdown () noexcept
{
    {
        const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
        if ( bShutdown )
        {
            return;
        }
        bShutdown = true;
    }
    Drain();
}
Lumen::USize Lumen::RHI::FRHIDeferredReleaseQueue::GetSize () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Entries.size();
}
Lumen::Bool Lumen::RHI::FRHIDeferredReleaseQueue::IsShutdown () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return bShutdown;
}
