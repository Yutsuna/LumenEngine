/**
 * @file RHIReadbackAllocator.cpp
 * @brief Implements mapped readback ring allocation over an RHI buffer.
 */

#include "RHIReadbackAllocator.hpp"

#include <Templates/Expected.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIReadbackAllocator::FRHIReadbackAllocator ( TSharedRef<IRHIBuffer> InBuffer, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FRHIReadbackAllocator>() ), Buffer( std::move( InBuffer ) ), Ring( Buffer->GetDesc().Size ),
      Capacity( static_cast<USize>( Buffer->GetDesc().Size ) )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI, FRHIReadbackAllocator )

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIReadbackAllocator::Allocate ( const FRHIAllocationDesc &InDesc ) noexcept
{
    FRHIAllocationDesc ReadbackDesc = InDesc;
    ReadbackDesc.MemoryClass        = ERHIMemoryClass::Readback;

    LUMEN_EXPECT( Result, Ring.Allocate( ReadbackDesc ) );
    IssuedIds.emplace( Result.Id.GetValue(), Result.Id );
    return Result;
}

Lumen::RHI::FRHIExpected
Lumen::RHI::FRHIReadbackAllocator::ValidateAllocation ( const FRHIAllocation &InAllocation ) const noexcept
{
    if ( not InAllocation.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback allocation is invalid" ) );
    }
    if ( InAllocation.MemoryClass != ERHIMemoryClass::Readback )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback mapping requires a Readback memory class" ) );
    }
    if ( not IssuedIds.contains( InAllocation.Id.GetValue() ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback allocation was not issued by this allocator" ) );
    }
    if ( InAllocation.Offset > Capacity or InAllocation.Size > Capacity - static_cast<USize>( InAllocation.Offset ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidMemory( "Readback range exceeds storage" ) );
    }
    return {};
}

Lumen::TExpected<Lumen::RHI::FRHIReadbackAllocation, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIReadbackAllocator::Map ( const FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_EXPECT( _, ValidateAllocation( InAllocation ) );

    return Buffer->Map( InAllocation.Offset, InAllocation.Size )
        .Transform( [&InAllocation] ( const TSpan<Byte> &InMapped ) noexcept -> FRHIReadbackAllocation
    {
        return FRHIReadbackAllocation{ .Allocation = InAllocation, .MappedData = InMapped };
    } );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIReadbackAllocator::GetBuffer ( const FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_EXPECT( _, ValidateAllocation( InAllocation ) );

    return Buffer;
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIReadbackAllocator::Free ( const FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_EXPECT( _, Ring.Free( InAllocation ) );

    IssuedIds.erase( InAllocation.Id.GetValue() );
    return {};
}
