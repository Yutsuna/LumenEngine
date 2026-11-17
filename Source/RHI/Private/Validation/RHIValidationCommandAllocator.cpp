/**
 * @file RHIValidationCommandAllocator.cpp
 * @brief Implements validated command allocator recycling.
 */
#include "RHIValidationCommandAllocator.hpp"
#include "RHIValidationCommandList.hpp"
#include <HAL/LockGuard.hpp>

Lumen::FRHIValidationCommandAllocator::FRHIValidationCommandAllocator ( TSharedRef<IRHICommandAllocator> InUnderlying,
                                                                        TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::CommandAllocator );
}

Lumen::FRHIValidationCommandAllocator::~FRHIValidationCommandAllocator () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationCommandAllocator )

Lumen::ERHIQueueType Lumen::FRHIValidationCommandAllocator::GetQueueType () const noexcept
{
    return Underlying->GetQueueType();
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationCommandAllocator::Reset ( const FRHITimelineValue InCompleted ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( const FRHIValidationCommandList *List : Lists )
    {
        if ( List->GetState() == ERHICommandListState::Pending and
             not IsRHICompletionReached( InCompleted, List->GetCompletion() ) )
        {
            return State->GetReporter()->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Command,
                                                 "Cannot recycle a command list before its completion", this );
        }
    }
    auto Result = Underlying->Reset( InCompleted );
    if ( Result )
    {
        for ( FRHIValidationCommandList *List : Lists )
        {
            List->OnAllocatorReset();
        }
    }
    return Result;
}

void Lumen::FRHIValidationCommandAllocator::RegisterList ( FRHIValidationCommandList &InList ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Lists.push_back( &InList );
}

void Lumen::FRHIValidationCommandAllocator::UnregisterList ( FRHIValidationCommandList &InList ) noexcept
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

Lumen::USize Lumen::FRHIValidationCommandAllocator::GetTrackedListCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Lists.size();
}
