/**
 * @file RHIMemoryAllocator.cpp
 * @brief Implements the budgeted allocator facade.
 */

#include <RHI/Memory/RHIMemoryAllocator.hpp>

Lumen::RHI::FRHIMemoryAllocator::FRHIMemoryAllocator ( IRHIAllocator &InAllocator, FRHIMemoryBudget &InBudget ) noexcept
    : Allocator( &InAllocator ), Budget( &InBudget )
{
    /* */
}

void Lumen::RHI::FRHIMemoryAllocator::RegisterObserver ( IRHIAllocationObserver &InObserver ) noexcept
{
    GetRHIAllocationObserverRegistry().Register( InObserver );
}

void Lumen::RHI::FRHIMemoryAllocator::UnregisterObserver ( IRHIAllocationObserver &InObserver ) noexcept
{
    GetRHIAllocationObserverRegistry().Unregister( InObserver );
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIMemoryAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    if ( not Budget->TryReserve( InDesc.Size ) )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Memory budget exhausted" ) );
    }
    return Allocator->Allocate( InDesc )
        .Inspect(
            [] ( const FRHIAllocation &InAllocation ) noexcept
    {
        GetRHIAllocationObserverRegistry().NotifyCreated( InAllocation );
    } )
        .OrElse( [this, &InDesc] ( const FRHIError &InError )
    {
        Budget->Release( InDesc.Size );
        return TExpected<FRHIAllocation, FRHIError>( TUnexpected<FRHIError>( InError ) );
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIMemoryAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Allocator->Free( InAllocation )
        .Inspect( [this, &InAllocation] () noexcept
    {
        Budget->Release( InAllocation.Size );
        GetRHIAllocationObserverRegistry().NotifyReleased( InAllocation );
    } );
}
