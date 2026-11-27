/**
 * @file RHIPoolAllocator.cpp
 * @brief Implements fixed-size slot allocation.
 */

#include <HAL/LockGuard.hpp>

#include <RHI/Memory/RHIPoolAllocator.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIPoolAllocator::FRHIPoolAllocator ( const UInt64 InSlotSize, const UInt32 InSlotCount ) noexcept
    : SlotSize( 0ULL ), Slots( MakeUnique<FIndiceHeapAllocator>( 0U ) )
{
    if ( not RHIIsPowerOfTwo( InSlotSize ) )
    {
        return;
    }
    const TOptional<UInt64> Capacity = RHICheckedMultiply( InSlotSize, static_cast<UInt64>( InSlotCount ) );
    if ( not Capacity )
    {
        return;
    }
    SlotSize       = InSlotSize;
    Stats.Capacity = *Capacity;
    Slots          = MakeUnique<FIndiceHeapAllocator>( InSlotCount );
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIPoolAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( _, InDesc.Validate() );

    if ( SlotSize == 0ULL )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidState( ERHIErrorCondition::Memory, "Pool allocator is disabled" ) );
    }
    if ( InDesc.Size > SlotSize or InDesc.Alignment > SlotSize )
    {
        return TUnexpected<FRHIError>(
            MakeRHIUnsupported( ERHIErrorCondition::Memory, "Allocation exceeds the pool slot class" ) );
    }

    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const TOptional<FIndiceAllocation> Slot = Slots->Allocate( 1U );

    if ( not Slot )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Memory, "Pool exhausted" ) );
    }

    /** INFO: A power-of-two SlotSize with Alignment <= SlotSize keeps every slot offset aligned. */
    const UInt64 Offset = static_cast<UInt64>( Slot->Start ) * SlotSize;
    if ( Offset % InDesc.Alignment != 0ULL )
    {
        Slots->Free( *Slot );
        return TUnexpected<FRHIError>(
            MakeRHIInvalidState( ERHIErrorCondition::Memory, "Pool offset violates requested alignment" ) );
    }

    Stats.OnAllocate( SlotSize );
    const FRHIAllocation Result{ .Id          = FRHIAllocationId{ NextId++ },
                                 .Offset      = Offset,
                                 .Size        = SlotSize,
                                 .Alignment   = SlotSize,
                                 .MemoryClass = InDesc.MemoryClass,
                                 .Heap        = nullptr };
    ActiveAllocations.push_back( Result.Id );
    return Result;
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIPoolAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() or SlotSize == 0ULL or InAllocation.Offset % SlotSize != 0ULL or
         InAllocation.Offset / SlotSize >= Stats.Capacity / SlotSize )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Invalid pool allocation" ) );
    }
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( USize Index = 0U; Index < ActiveAllocations.size(); ++Index )
    {
        if ( ActiveAllocations[Index] == InAllocation.Id )
        {
            ActiveAllocations.erase( ActiveAllocations.begin() + static_cast<ISize>( Index ) );
            Slots->Free( FIndiceAllocation( static_cast<UInt32>( InAllocation.Offset / SlotSize ), 1U ) );
            Stats.OnFree( SlotSize );
            return {};
        }
    }
    return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Pool allocation is not active" ) );
}

void Lumen::RHI::FRHIPoolAllocator::Reset () noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const UInt32 Count = static_cast<UInt32>( SlotSize == 0ULL ? 0ULL : Stats.Capacity / SlotSize );

    Slots = MakeUnique<FIndiceHeapAllocator>( Count );
    ActiveAllocations.clear();
    Stats.Used = 0ULL;
}

Lumen::RHI::FRHIAllocatorStats Lumen::RHI::FRHIPoolAllocator::GetStats () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
