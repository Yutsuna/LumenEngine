/**
 * @file VulkanHeap.cpp
 * @brief Implements Vulkan device-memory heap creation, placement support and mapping.
 */

#include <RHIVulkan/Memory/VulkanHeap.hpp>

#include <RHIVulkan/Memory/VulkanMemory.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

#include <RHIVulkan/Types/VulkanObject.hpp>

#include <HAL/LockGuard.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanHeap>, Lumen::RHI::FRHIError> Lumen::RHIVulkan::FVulkanHeap::Create (
    TSharedRef<FVulkanDeviceContext> InContext, const RHI::FRHIHeapDesc &InDesc, RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );

    const TOptional<UInt32> MemoryTypeIndex =
        SelectVulkanHeapMemoryTypeIndex( InContext->GetPhysicalDevice(), InDesc.Size, InDesc.MemoryClass );
    if ( not MemoryTypeIndex.HasValue() )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Memory,
                                                     "No Vulkan memory type satisfies the requested heap size and class." ) );
    }

    VkMemoryAllocateInfo AllocateInfo{};
    AllocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    AllocateInfo.allocationSize  = InDesc.Size;
    AllocateInfo.memoryTypeIndex = *MemoryTypeIndex;

    TSharedRef<FVulkanHeap> Heap =
        MakeShared<FVulkanHeap>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId, *MemoryTypeIndex );
    LUMEN_VK_TRY( vkAllocateMemory( InContext->GetDevice(), &AllocateInfo, nullptr,
                                    Heap->Memory.ResetAndGetAddressOf( FVulkanDeviceMemoryDeleter{ InContext->GetDevice() } ) ),
                  RHI::ERHIErrorCondition::Memory );
    return Heap;
}

Lumen::RHIVulkan::FVulkanHeap::FVulkanHeap ( TSharedRef<FVulkanDeviceContext> InContext,
                                             const RHI::FRHIHeapDesc &InDesc,
                                             RHI::FRHIObjectId InId,
                                             UInt32 InMemoryTypeIndex ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanHeap>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), Desc( InDesc ),
      MemoryTypeIndex( InMemoryTypeIndex )
{
    /* */
}

Lumen::RHIVulkan::FVulkanHeap::~FVulkanHeap () noexcept
{
    Unmap();
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanHeap )

const Lumen::RHI::FRHIHeapDesc &Lumen::RHIVulkan::FVulkanHeap::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanHeap::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    HAL::TUniqueLock Lock( MappingMutex );
    if ( not RHI::IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Vulkan heap mapping is outside its range or not CPU visible." ) );
    }
    if ( MappedBase == nullptr )
    {
        void *NativePointer = nullptr;
        LUMEN_VK_TRY( vkMapMemory( Context->GetDevice(), Memory.Get(), 0ULL, VK_WHOLE_SIZE, 0U, &NativePointer ),
                      RHI::ERHIErrorCondition::Memory );
        MappedBase = static_cast<Byte *>( NativePointer );
    }
    return TSpan<Byte>( MappedBase + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}

void Lumen::RHIVulkan::FVulkanHeap::Unmap () noexcept
{
    HAL::TUniqueLock Lock( MappingMutex );
    if ( MappedBase != nullptr )
    {
        vkUnmapMemory( Context->GetDevice(), Memory.Get() );
        MappedBase = nullptr;
    }
}

VkDeviceMemory Lumen::RHIVulkan::FVulkanHeap::GetNativeMemory () const noexcept
{
    return Memory.Get();
}

Lumen::UInt32 Lumen::RHIVulkan::FVulkanHeap::GetMemoryTypeIndex () const noexcept
{
    return MemoryTypeIndex;
}
