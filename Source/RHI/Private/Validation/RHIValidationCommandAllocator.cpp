/**
 * @file RHIValidationCommandAllocator.cpp
 * @brief Implements validated command allocator recycling.
 */

#include "RHIValidationCommandAllocator.hpp"
#include "RHIValidationCommandList.hpp"

#include <HAL/LockGuard.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIValidationCommandAllocator::FRHIValidationCommandAllocator ( TSharedRef<IRHICommandAllocator> InUnderlying,
                                                                             TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::CommandAllocator );
}

Lumen::RHI::FRHIValidationCommandAllocator::~FRHIValidationCommandAllocator () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationCommandAllocator )

Lumen::RHI::ERHIQueueType Lumen::RHI::FRHIValidationCommandAllocator::GetQueueType () const noexcept
{
    return Underlying->GetQueueType();
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationCommandAllocator::Reset ( const FRHITimelineValue InCompleted ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( const FRHIValidationCommandList *List : Lists )
    {
        if ( List->GetState() == ERHICommandListState::Pending and
             not IsRHICompletionReached( InCompleted, List->GetCompletion() ) )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                                 "Cannot recycle a command list before its completion", this );
        }
    }
    return Underlying->Reset( InCompleted )
        .Inspect( [this] () noexcept
    {
        for ( FRHIValidationCommandList *List : Lists )
        {
            List->OnAllocatorReset();
        }
    } );
}

void Lumen::RHI::FRHIValidationCommandAllocator::RegisterList ( FRHIValidationCommandList &InList ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Lists.push_back( &InList );
}

void Lumen::RHI::FRHIValidationCommandAllocator::UnregisterList ( FRHIValidationCommandList &InList ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( USize Index = 0U; Index < Lists.size(); ++Index )
    {
        if ( Lists[Index] == &InList )
        {
            Lists.erase( Lists.begin() + static_cast<ISize>( Index ) );
            return;
        }
    }
}

Lumen::USize Lumen::RHI::FRHIValidationCommandAllocator::GetTrackedListCount () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Lists.size();
}
