/**
 * @file RHICommandAllocatorBase.cpp
 * @brief Implements tracked command-list recycling.
 */

#include <HAL/LockGuard.hpp>
#include <RHI/Commands/RHICommandAllocatorBase.hpp>
Lumen::RHI::FRHICommandAllocatorBase::FRHICommandAllocatorBase ( const ERHIQueueType InQueueType ) noexcept
    : QueueType( InQueueType )
{
}
Lumen::RHI::ERHIQueueType Lumen::RHI::FRHICommandAllocatorBase::GetQueueType () const noexcept
{
    return QueueType;
}
Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHICommandAllocatorBase::Reset ( const FRHITimelineValue InCompleted ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( IRHICommandList *List : Lists )
    {
        if ( List->GetState() == ERHICommandListState::Pending and
             not IsRHICompletionReached( InCompleted, List->GetCompletion() ) )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidState( ERHIErrorCondition::Command, "Cannot recycle pending command lists" ) );
        }
    }
    for ( IRHICommandList *List : Lists )
    {
        if ( List->GetState() == ERHICommandListState::Pending )
        {
            LUMEN_EXPECT( _, List->MarkCompleted( InCompleted ) );
        }
        if ( List->GetState() != ERHICommandListState::Initial )
        {
            LUMEN_EXPECT( _, List->Reset() );
        }
    }
    return {};
}
void Lumen::RHI::FRHICommandAllocatorBase::RegisterList ( IRHICommandList &InList ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Lists.push_back( &InList );
}
void Lumen::RHI::FRHICommandAllocatorBase::UnregisterList ( IRHICommandList &InList ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( USize Index = 0U; Index < Lists.size(); ++Index )
    {
        if ( Lists[Index] == &InList )
        {
            Lists.erase( Lists.begin() + static_cast<ISize>( Index ) );
            return;
        }
    }
}
Lumen::USize Lumen::RHI::FRHICommandAllocatorBase::GetTrackedListCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Lists.size();
}
