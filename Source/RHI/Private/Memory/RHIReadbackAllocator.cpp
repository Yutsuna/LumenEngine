/**
 * @file RHIReadbackAllocator.cpp
 * @brief Implements mapped readback ring allocation.
 */
#include "RHIReadbackAllocator.hpp"
#include <Templates/Hash.hpp>
Lumen::FRHIReadbackAllocator::FRHIReadbackAllocator ( const UInt64 InCapacity, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIReadbackAllocator>() ), Ring( InCapacity ),
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InCapacity ) ) ), Capacity( static_cast<USize>( InCapacity ) )
{
}
LUMEN_RHI_OBJECT_FORWARDERS( FRHIReadbackAllocator )
Lumen::TExpected<Lumen::FRHIAllocation, Lumen::FRHIError>
Lumen::FRHIReadbackAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    FRHIAllocationDesc ReadbackDesc = InDesc;
    ReadbackDesc.MemoryClass        = ERHIMemoryClass::Readback;
    return Ring.Allocate( ReadbackDesc );
}
Lumen::TExpected<Lumen::FRHIReadbackAllocation, Lumen::FRHIError>
Lumen::FRHIReadbackAllocator::Map ( const FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() or InAllocation.Offset > Capacity or
         InAllocation.Size > Capacity - static_cast<USize>( InAllocation.Offset ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback range exceeds storage" ) );
    }
    return FRHIReadbackAllocation{ InAllocation, TSpan<const Byte>( Storage.Get() + static_cast<USize>( InAllocation.Offset ),
                                                                    static_cast<USize>( InAllocation.Size ) ) };
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIReadbackAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}
