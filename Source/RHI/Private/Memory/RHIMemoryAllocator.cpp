/**
 * @file RHIMemoryAllocator.cpp
 * @brief Implements the budgeted allocator facade.
 */
#include <Memory/RHIMemoryAllocator.hpp>
Lumen::FRHIMemoryAllocator::FRHIMemoryAllocator ( IRHIAllocator &InAllocator, FRHIMemoryBudget &InBudget ) noexcept
    : Allocator( &InAllocator ), Budget( &InBudget )
{
}
Lumen::TExpected<Lumen::FRHIAllocation, Lumen::FRHIError>
Lumen::FRHIMemoryAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    if ( not Budget->TryReserve( InDesc.Size ) )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Memory budget exhausted" ) );
    }
    return Allocator->Allocate( InDesc ).OrElse( [this, &InDesc] ( const FRHIError &InError )
    {
        Budget->Release( InDesc.Size );
        return TExpected<FRHIAllocation, FRHIError>( TUnexpected<FRHIError>( InError ) );
    } );
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIMemoryAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    const auto Result = Allocator->Free( InAllocation );
    if ( Result )
    {
        Budget->Release( InAllocation.Size );
    }
    return Result;
}
