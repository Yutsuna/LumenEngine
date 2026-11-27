/**
 * @file VulkanStagingAllocator.cpp
 * @brief Implements Vulkan upload and readback allocators over persistent staging buffers.
 */

#include <RHIVulkan/Memory/VulkanStagingAllocator.hpp>

#include <RHIVulkan/Memory/VulkanMemory.hpp>
#include <RHIVulkan/Resources/VulkanBuffer.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>

#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHIResourceUsage.hpp>

/**
 * Public
 */

Lumen::RHIVulkan::FVulkanStagingRing::FVulkanStagingRing ( TSharedRef<FVulkanBuffer> InBuffer,
                                                           RHI::ERHIMemoryClass InMemoryClass ) noexcept
    : Buffer( static_cast<TSharedRef<FVulkanBuffer> &&>( InBuffer ) ), Allocator( InBuffer->GetDesc().Size ),
      MemoryClass( InMemoryClass )
{
    /* */
}

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanStagingRing::Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Allocation, Allocator.Allocate( InDesc ) );
    Allocation.MemoryClass = MemoryClass;
    return Allocation;
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanStagingRing::Free ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    return Allocator.Free( InAllocation );
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanStagingRing::GetMappedData ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not Owns( InAllocation ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Staging allocation is not owned by this allocator." ) );
    }
    return Buffer->Map( InAllocation.Offset, InAllocation.Size );
}

const Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanBuffer> &Lumen::RHIVulkan::FVulkanStagingRing::GetBuffer () const noexcept
{
    return Buffer;
}

Lumen::Bool Lumen::RHIVulkan::FVulkanStagingRing::Owns ( const RHI::FRHIAllocation &InAllocation ) const noexcept
{
    const UInt64 Capacity = Buffer->GetDesc().Size;
    return InAllocation.IsValid() and InAllocation.MemoryClass == MemoryClass and InAllocation.Offset < Capacity and
           InAllocation.Size <= Capacity - InAllocation.Offset;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanUploadAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanUploadAllocator::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                                   const UInt64 InCapacity,
                                                   RHI::FRHIObjectId InId,
                                                   RHI::FRHIObjectId InBufferId ) noexcept
{
    const RHI::FRHIBufferDesc BufferDesc{
        .Size = InCapacity, .Usage = RHI::ERHIResourceUsage::CopySource, .MemoryClass = RHI::ERHIMemoryClass::Upload };
    TExpected<TSharedRef<FVulkanBuffer>, RHI::FRHIError> Buffer = FVulkanBuffer::Create( InContext, BufferDesc, InBufferId );
    if ( not Buffer )
    {
        return TUnexpected( Buffer.Error() );
    }
    return MakeShared<FVulkanUploadAllocator>( static_cast<TSharedRef<FVulkanBuffer> &&>( *Buffer ), InId );
}

Lumen::RHIVulkan::FVulkanUploadAllocator::FVulkanUploadAllocator ( TSharedRef<FVulkanBuffer> InBuffer,
                                                                   RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanUploadAllocator>( InId ) ),
      Ring( static_cast<TSharedRef<FVulkanBuffer> &&>( InBuffer ), RHI::ERHIMemoryClass::Upload )
{
    /* */
}

Lumen::RHIVulkan::FVulkanUploadAllocator::~FVulkanUploadAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanUploadAllocator )

Lumen::TExpected<Lumen::RHI::FRHIUploadAllocation, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanUploadAllocator::Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Allocation, Ring.Allocate( InDesc ) );
    LUMEN_EXPECT( Mapped, Ring.GetMappedData( Allocation ) );
    return RHI::FRHIUploadAllocation{ .Allocation = Allocation, .Buffer = Ring.GetBuffer(), .MappedData = Mapped };
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanUploadAllocator::Free ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanReadbackAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanReadbackAllocator::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                                     const UInt64 InCapacity,
                                                     RHI::FRHIObjectId InId,
                                                     RHI::FRHIObjectId InBufferId ) noexcept
{
    const RHI::FRHIBufferDesc BufferDesc{
        .Size = InCapacity, .Usage = RHI::ERHIResourceUsage::CopyDestination, .MemoryClass = RHI::ERHIMemoryClass::Readback };
    TExpected<TSharedRef<FVulkanBuffer>, RHI::FRHIError> Buffer = FVulkanBuffer::Create( InContext, BufferDesc, InBufferId );
    if ( not Buffer )
    {
        return TUnexpected( Buffer.Error() );
    }
    return MakeShared<FVulkanReadbackAllocator>( static_cast<TSharedRef<FVulkanBuffer> &&>( *Buffer ), InId );
}

Lumen::RHIVulkan::FVulkanReadbackAllocator::FVulkanReadbackAllocator ( TSharedRef<FVulkanBuffer> InBuffer,
                                                                       RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanReadbackAllocator>( InId ) ),
      Ring( static_cast<TSharedRef<FVulkanBuffer> &&>( InBuffer ), RHI::ERHIMemoryClass::Readback )
{
    /* */
}

Lumen::RHIVulkan::FVulkanReadbackAllocator::~FVulkanReadbackAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanReadbackAllocator )

Lumen::TExpected<Lumen::RHI::FRHIAllocation, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanReadbackAllocator::Allocate ( const RHI::FRHIAllocationDesc &InDesc ) noexcept
{
    return Ring.Allocate( InDesc );
}

Lumen::TExpected<Lumen::RHI::FRHIReadbackAllocation, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanReadbackAllocator::Map ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    LUMEN_EXPECT( Mapped, Ring.GetMappedData( InAllocation ) );
    return RHI::FRHIReadbackAllocation{ .Allocation = InAllocation, .MappedData = Mapped };
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanReadbackAllocator::GetBuffer ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not Ring.Owns( InAllocation ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Readback allocation is not owned by this allocator." ) );
    }
    return Ring.GetBuffer();
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanReadbackAllocator::Free ( const RHI::FRHIAllocation &InAllocation ) noexcept
{
    return Ring.Free( InAllocation );
}
