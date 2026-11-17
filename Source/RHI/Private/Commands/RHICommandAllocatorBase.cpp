/**
 * @file RHICommandAllocatorBase.cpp
 * @brief Implements tracked command-list recycling.
 */

#include <Commands/RHICommandAllocatorBase.hpp>
#include <HAL/LockGuard.hpp>
Lumen::FRHICommandAllocatorBase::FRHICommandAllocatorBase ( const ERHIQueueType InQueueType ) noexcept : QueueType( InQueueType )
{
}
Lumen::ERHIQueueType Lumen::FRHICommandAllocatorBase::GetQueueType () const noexcept
{
    return QueueType;
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHICommandAllocatorBase::Reset ( const FRHITimelineValue InCompleted ) noexcept
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
            const auto Completed = List->MarkCompleted( InCompleted );
            if ( not Completed )
            {
                return TUnexpected<FRHIError>( Completed.Error() );
            }
        }
        if ( List->GetState() != ERHICommandListState::Initial )
        {
            const auto ResetResult = List->Reset();
            if ( not ResetResult )
            {
                return TUnexpected<FRHIError>( ResetResult.Error() );
            }
        }
    }
    return {};
}
void Lumen::FRHICommandAllocatorBase::RegisterList ( IRHICommandList &InList ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Lists.push_back( &InList );
}
void Lumen::FRHICommandAllocatorBase::UnregisterList ( IRHICommandList &InList ) noexcept
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
Lumen::USize Lumen::FRHICommandAllocatorBase::GetTrackedListCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Lists.size();
}
