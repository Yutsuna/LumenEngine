/**
 * @file RHIUploadAllocator.cpp
 * @brief Implements mapped upload ring allocation over an RHI buffer.
 */

#include "RHIUploadAllocator.hpp"
#include <RHI/Utilities/RHICheckedMath.hpp>
#include <Templates/Hash.hpp>

Lumen::RHI::FRHIUploadAllocator::FRHIUploadAllocator ( TSharedRef<IRHIBuffer> InBuffer, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIUploadAllocator>() ), Buffer( std::move( InBuffer ) ), Ring( Buffer->GetDesc().Size ),
      Capacity( static_cast<USize>( Buffer->GetDesc().Size ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI, FRHIUploadAllocator )

Lumen::TExpected<Lumen::RHI::FRHIUploadAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIUploadAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    FRHIAllocationDesc UploadDesc = InDesc;
    UploadDesc.MemoryClass        = ERHIMemoryClass::Upload;
    return Ring.Allocate( UploadDesc )
        .AndThen( [this] ( const FRHIAllocation &InAllocation ) noexcept -> TExpected<FRHIUploadAllocation, FRHIError>
    {
        const TOptional<USize> End =
            RHICheckedAdd( static_cast<USize>( InAllocation.Offset ), static_cast<USize>( InAllocation.Size ) );
        if ( not End or *End > Capacity )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Upload range exceeds storage" ) );
        }
        TExpected<TSpan<Byte>, FRHIError> Mapped = Buffer->Map( InAllocation.Offset, InAllocation.Size );
        if ( not Mapped )
        {
            Ring.Free( InAllocation );
            return TUnexpected<FRHIError>( Mapped.Error() );
        }
        return FRHIUploadAllocation{ .Allocation = InAllocation, .Buffer = Buffer, .MappedData = *Mapped };
    } );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIUploadAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}
