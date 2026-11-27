/**
 * @file VulkanBuffer.cpp
 * @brief Implements Vulkan buffer creation, placement, native binding and mapping.
 */

#include <RHIVulkan/Resources/VulkanBuffer.hpp>

#include <RHIVulkan/Memory/VulkanHeap.hpp>
#include <RHIVulkan/Memory/VulkanMemory.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>
#include <RHIVulkan/Types/VulkanTypeMapping.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <RHI/Device/RHIBackendCast.hpp>

#include <HAL/LockGuard.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanBuffer::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                          const RHI::FRHIBufferDesc &InDesc,
                                          RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    if ( InDesc.Size > InContext->GetLimits().MaxBufferSize )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Requested buffer size exceeds the published device limit." ) );
    }

    const TExpected<VkBufferUsageFlags, RHI::FRHIError> NativeUsage = ToVulkanBufferUsage( InDesc.Usage );
    LUMEN_EXPECT( Usage, NativeUsage );

    TSharedRef<FVulkanBuffer> Buffer =
        MakeShared<FVulkanBuffer>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId );

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = InDesc.Size;
    BufferInfo.usage       = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    LUMEN_VK_TRY( vkCreateBuffer( Buffer->Context->GetDevice(), &BufferInfo, nullptr,
                                  Buffer->Buffer.ResetAndGetAddressOf( FVulkanBufferDeleter{ Buffer->Context->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Resource );

    VkMemoryRequirements Requirements{};
    vkGetBufferMemoryRequirements( Buffer->Context->GetDevice(), Buffer->Buffer.Get(), &Requirements );
    Buffer->Requirements.Size      = static_cast<UInt64>( Requirements.size );
    Buffer->Requirements.Alignment = static_cast<UInt64>( Requirements.alignment );
    Buffer->Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );

    TExpected<FVulkanDeviceMemoryHandle, RHI::FRHIError> Memory = AllocateVulkanMemory(
        Buffer->Context->GetPhysicalDevice(), Buffer->Context->GetDevice(), Requirements, InDesc.MemoryClass );
    LUMEN_EXPECT( AllocatedMemory, Memory );
    Buffer->OwnMemory = static_cast<FVulkanDeviceMemoryHandle &&>( AllocatedMemory );

    LUMEN_VK_TRY( vkBindBufferMemory( Buffer->Context->GetDevice(), Buffer->Buffer.Get(), Buffer->OwnMemory.Get(), 0ULL ),
                  RHI::ERHIErrorCondition::Memory );
    return Buffer;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanBuffer::CreatePlaced ( TSharedRef<FVulkanDeviceContext> InContext,
                                                const RHI::FRHIBufferDesc &InDesc,
                                                TSharedRef<RHI::IRHIHeap> InHeap,
                                                const RHI::FRHIAllocation &InAllocation,
                                                RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );

    const TOptional<FVulkanHeap *> NativeHeap = RHI::TryRHIBackendCast<FVulkanHeap>( *InHeap );
    if ( not NativeHeap.HasValue() or InAllocation.Heap != InHeap.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument(
            RHI::ERHIErrorCondition::Memory, "Placed buffer requires a Vulkan heap and a matching allocation owner." ) );
    }
    if ( InAllocation.MemoryClass != InDesc.MemoryClass or not InAllocation.IsValid() )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                         "Placed buffer allocation has an incompatible memory class." ) );
    }
    if ( InAllocation.Alignment == 0ULL or ( InAllocation.Alignment & ( InAllocation.Alignment - 1ULL ) ) != 0ULL or
         InAllocation.Offset % InAllocation.Alignment != 0ULL )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Placed buffer allocation offset is misaligned." ) );
    }
    if ( InAllocation.Size < InDesc.Size or InAllocation.Offset > InHeap->GetDesc().Size or
         InDesc.Size > InHeap->GetDesc().Size - InAllocation.Offset )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory, "Placed buffer allocation lies outside its heap." ) );
    }

    const TExpected<VkBufferUsageFlags, RHI::FRHIError> NativeUsage = ToVulkanBufferUsage( InDesc.Usage );
    LUMEN_EXPECT( Usage, NativeUsage );

    TSharedRef<FVulkanBuffer> Buffer =
        MakeShared<FVulkanBuffer>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId );

    VkBufferCreateInfo BufferInfo{};
    BufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    BufferInfo.size        = InDesc.Size;
    BufferInfo.usage       = Usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    LUMEN_VK_TRY( vkCreateBuffer( Buffer->Context->GetDevice(), &BufferInfo, nullptr,
                                  Buffer->Buffer.ResetAndGetAddressOf( FVulkanBufferDeleter{ Buffer->Context->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Resource );

    VkMemoryRequirements Requirements{};
    vkGetBufferMemoryRequirements( Buffer->Context->GetDevice(), Buffer->Buffer.Get(), &Requirements );
    if ( InAllocation.Offset % static_cast<UInt64>( Requirements.alignment ) != 0ULL )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                         "Placed buffer offset does not satisfy native alignment." ) );
    }
    if ( static_cast<UInt64>( Requirements.size ) > InAllocation.Size )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                         "Placed buffer allocation is smaller than native requirements." ) );
    }
    if ( ( Requirements.memoryTypeBits & ( 1U << ( *NativeHeap )->GetMemoryTypeIndex() ) ) == 0U )
    {
        return TUnexpected( RHI::MakeRHIInvalidArgument(
            RHI::ERHIErrorCondition::Memory, "Placed buffer heap is incompatible with native memory requirements." ) );
    }

    Buffer->Requirements.Size      = static_cast<UInt64>( Requirements.size );
    Buffer->Requirements.Alignment = static_cast<UInt64>( Requirements.alignment );
    Buffer->Requirements.PreferredMemoryClass.Emplace( InDesc.MemoryClass );
    Buffer->Heap              = static_cast<TSharedPtr<RHI::IRHIHeap>>( InHeap );
    Buffer->Allocation        = InAllocation;
    Buffer->MappingBaseOffset = InAllocation.Offset;

    LUMEN_VK_TRY( vkBindBufferMemory( Buffer->Context->GetDevice(), Buffer->Buffer.Get(), ( *NativeHeap )->GetNativeMemory(),
                                      InAllocation.Offset ),
                  RHI::ERHIErrorCondition::Memory );
    return Buffer;
}

Lumen::RHIVulkan::FVulkanBuffer::FVulkanBuffer ( TSharedRef<FVulkanDeviceContext> InContext,
                                                 const RHI::FRHIBufferDesc &InDesc,
                                                 RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanBuffer>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIVulkan::FVulkanBuffer::~FVulkanBuffer () noexcept
{
    Unmap();
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanBuffer )

const Lumen::RHI::FRHIBufferDesc &Lumen::RHIVulkan::FVulkanBuffer::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanBuffer::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    HAL::TUniqueLock Lock( MappingMutex );
    if ( not RHI::IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Vulkan buffer mapping is outside its range or not CPU visible." ) );
    }
    if ( MappedBase == nullptr )
    {
        void *NativePointer = nullptr;
        LUMEN_VK_TRY( vkMapMemory( Context->GetDevice(), GetNativeMemory(), 0ULL, VK_WHOLE_SIZE, 0U, &NativePointer ),
                      RHI::ERHIErrorCondition::Resource );
        MappedBase = static_cast<Byte *>( NativePointer );
    }
    return TSpan<Byte>( MappedBase + static_cast<USize>( MappingBaseOffset + InOffset ), static_cast<USize>( InSize ) );
}

void Lumen::RHIVulkan::FVulkanBuffer::Unmap () noexcept
{
    HAL::TUniqueLock Lock( MappingMutex );
    if ( MappedBase != nullptr )
    {
        vkUnmapMemory( Context->GetDevice(), GetNativeMemory() );
        MappedBase = nullptr;
    }
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHIVulkan::FVulkanBuffer::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHIVulkan::FVulkanBuffer::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHIVulkan::FVulkanBuffer::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHIVulkan::FVulkanBuffer::GetLifetime () const noexcept
{
    return {};
}

VkBuffer Lumen::RHIVulkan::FVulkanBuffer::GetNativeBuffer () const noexcept
{
    return Buffer.Get();
}

Lumen::Bool Lumen::RHIVulkan::FVulkanBuffer::IsPlaced () const noexcept
{
    return Heap.Get() != nullptr;
}

VkDeviceMemory Lumen::RHIVulkan::FVulkanBuffer::GetNativeMemory () const noexcept
{
    return Heap.Get() != nullptr ? static_cast<FVulkanHeap *>( Heap.Get() )->GetNativeMemory() : OwnMemory.Get();
}
