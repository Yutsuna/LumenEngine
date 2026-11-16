/**
 * @file RHIPoolAllocator.cpp
 * @brief Implements fixed-size slot allocation.
 */
#include <HAL/LockGuard.hpp>
#include <Memory/RHIPoolAllocator.hpp>
#include <Utilities/RHICheckedMath.hpp>
Lumen::FRHIPoolAllocator::FRHIPoolAllocator ( const UInt64 InSlotSize, const UInt32 InSlotCount ) noexcept
    : SlotSize( InSlotSize ), Slots( MakeUnique<FIndiceHeapAllocator>( InSlotCount ) )
{
    Stats.Capacity = InSlotSize * static_cast<UInt64>( InSlotCount );
}
Lumen::TExpected<Lumen::FRHIAllocation, Lumen::FRHIError>
Lumen::FRHIPoolAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    if ( const auto Valid = InDesc.Validate(); not Valid )
    {
        return TUnexpected<FRHIError>( Valid.Error() );
    }
    if ( InDesc.Size > SlotSize or InDesc.Alignment > SlotSize )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::Unsupported, ERHIErrorCondition::Memory, "Allocation exceeds the pool slot class" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Slot = Slots->Allocate( 1U );
    if ( not Slot )
    {
        return TUnexpected<FRHIError>( FRHIError( ERHIErrorCode::OutOfMemory, ERHIErrorCondition::Memory, "Pool exhausted" ) );
    }
    Stats.OnAllocate( SlotSize );
    return FRHIAllocation{ FRHIAllocationId{ NextId++ },
                           static_cast<UInt64>( Slot->Start ) * SlotSize,
                           SlotSize,
                           SlotSize,
                           InDesc.MemoryClass,
                           nullptr };
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIPoolAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() or SlotSize == 0ULL or InAllocation.Offset % SlotSize != 0ULL or
         InAllocation.Offset / SlotSize >= Stats.Capacity / SlotSize )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Memory, "Invalid pool allocation" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Slots->Free( FIndiceAllocation( static_cast<UInt32>( InAllocation.Offset / SlotSize ), 1U ) );
    Stats.OnFree( SlotSize );
    return {};
}
void Lumen::FRHIPoolAllocator::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const UInt32 Count = static_cast<UInt32>( SlotSize == 0ULL ? 0ULL : Stats.Capacity / SlotSize );
    Slots              = MakeUnique<FIndiceHeapAllocator>( Count );
    Stats.Used         = 0ULL;
}
Lumen::FRHIAllocatorStats Lumen::FRHIPoolAllocator::GetStats () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return Stats;
}
