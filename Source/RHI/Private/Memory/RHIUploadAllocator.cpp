/**
 * @file RHIUploadAllocator.cpp
 * @brief Implements mapped upload ring allocation.
 */
#include "RHIUploadAllocator.hpp"
#include <Templates/Hash.hpp>
#include <Utilities/RHICheckedMath.hpp>
Lumen::FRHIUploadAllocator::FRHIUploadAllocator ( const UInt64 InCapacity, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIUploadAllocator>() ), Ring( InCapacity ),
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InCapacity ) ) ), Capacity( static_cast<USize>( InCapacity ) )
{
}
LUMEN_RHI_OBJECT_FORWARDERS( FRHIUploadAllocator )
Lumen::TExpected<Lumen::FRHIUploadAllocation, Lumen::FRHIError>
Lumen::FRHIUploadAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    FRHIAllocationDesc UploadDesc = InDesc;
    UploadDesc.MemoryClass        = ERHIMemoryClass::Upload;
    auto Allocation               = Ring.Allocate( UploadDesc );
    if ( not Allocation )
    {
        return TUnexpected<FRHIError>( Allocation.Error() );
    }
    const auto End = RHICheckedAdd( static_cast<USize>( Allocation->Offset ), static_cast<USize>( Allocation->Size ) );
    if ( not End or *End > Capacity )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Memory, "Upload range exceeds storage" ) );
    }
    return FRHIUploadAllocation{ *Allocation, TSpan<Byte>( Storage.Get() + static_cast<USize>( Allocation->Offset ),
                                                           static_cast<USize>( Allocation->Size ) ) };
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIUploadAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}
