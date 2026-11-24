/**
 * @file RHILinearAllocator.cpp
 * @brief Implements bounded bump allocation.
 */

#include <HAL/LockGuard.hpp>

#include <RHI/Memory/RHILinearAllocator.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::RHI::FRHILinearAllocator::FRHILinearAllocator ( const UInt64 InCapacity ) noexcept : Capacity( InCapacity )
{
    Stats.Capacity = InCapacity;
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHILinearAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( _, InDesc.Validate() );

    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TOptional<UInt64> Offset = RHIAlignUp( Current, InDesc.Alignment );
    if ( not Offset )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Linear alignment overflow" ) );
    }

    const TOptional<UInt64> End = RHICheckedAdd( *Offset, InDesc.Size );
    if ( not End or *End > Capacity )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Linear allocator exhausted" ) );
    }

    Current = *End;
    Stats.OnAllocate( InDesc.Size );
    return FRHIAllocation{ .Id          = FRHIAllocationId{ NextId++ },
                           .Offset      = *Offset,
                           .Size        = InDesc.Size,
                           .Alignment   = InDesc.Alignment,
                           .MemoryClass = InDesc.MemoryClass,
                           .Heap        = nullptr };
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHILinearAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid linear allocation" ) );
    }
    return {};
}

void Lumen::RHI::FRHILinearAllocator::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Current    = 0ULL;
    Stats.Used = 0ULL;
}

Lumen::RHI::FRHIAllocatorStats Lumen::RHI::FRHILinearAllocator::GetStats () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
