/**
 * @file VulkanMemoryTest.cpp
 * @brief Functional unit tests for LumenEngine::VulkanRHI::FVulkanMemory dynamic descriptor and buffer allocation.
 */

#include "Vulkan/VulkanMemory.hpp"
#include "Vulkan/GPUDriven/GPUGlobalUniforms.hpp"
#include "Vulkan/VulkanBuffer.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <cstring>
#include <gtest/gtest.h>

namespace
{

LumenEngine::UInt32 GBufferCount = 0U;
LumenEngine::UInt8 GMockMappedMemory[4U][256U];

inline void ResetMockState () noexcept
{
    GBufferCount = 0U;
    std::memset( GMockMappedMemory, 0, sizeof( GMockMappedMemory ) );
}

} // namespace

extern "C"
{
    VkResult vmaCreateAllocator ( const VmaAllocatorCreateInfo *InCreateInfo, VmaAllocator *OutAllocator )
    {
        ( void )InCreateInfo;
        *OutAllocator = reinterpret_cast<VmaAllocator>( 0x1234ULL );
        return VK_SUCCESS;
    }

    void vmaDestroyAllocator ( VmaAllocator InAllocator )
    {
        ( void )InAllocator;
    }

    VkResult vmaCreateBuffer ( VmaAllocator InAllocator,
                               const VkBufferCreateInfo *InBufferInfo,
                               const VmaAllocationCreateInfo *InAllocationInfo,
                               VkBuffer *OutBuffer,
                               VmaAllocation *OutAllocation,
                               VmaAllocationInfo *OutAllocationInfo )
    {
        ( void )InAllocator;
        ( void )InAllocationInfo;

        const uint32_t Index = GBufferCount % 4U;
        ++GBufferCount;

        *OutBuffer     = reinterpret_cast<VkBuffer>( static_cast<uintptr_t>( 0x5678U + Index ) );
        *OutAllocation = reinterpret_cast<VmaAllocation>( static_cast<uintptr_t>( 0x9abcU + Index ) );

        if ( OutAllocationInfo != nullptr )
        {
            OutAllocationInfo->offset      = 0U;
            OutAllocationInfo->size        = InBufferInfo->size;
            OutAllocationInfo->pMappedData = GMockMappedMemory[Index];
        }

        return VK_SUCCESS;
    }

    void vmaDestroyBuffer ( VmaAllocator InAllocator, VkBuffer InBuffer, VmaAllocation InAllocation )
    {
        ( void )InAllocator;
        ( void )InBuffer;
        ( void )InAllocation;
    }

    VkResult vmaFlushAllocation ( VmaAllocator InAllocator, VmaAllocation InAllocation, VkDeviceSize InOffset, VkDeviceSize InSize )
    {
        ( void )InAllocator;
        ( void )InAllocation;
        ( void )InOffset;
        ( void )InSize;
        return VK_SUCCESS;
    }

    VkResult vkCreateDescriptorSetLayout ( VkDevice InDevice,
                                           const VkDescriptorSetLayoutCreateInfo *InCreateInfo,
                                           const VkAllocationCallbacks *InAllocator,
                                           VkDescriptorSetLayout *OutSetLayout )
    {
        ( void )InDevice;
        ( void )InCreateInfo;
        ( void )InAllocator;

        static uintptr_t SLayoutCounter = 0x1111ULL;
        *OutSetLayout                   = reinterpret_cast<VkDescriptorSetLayout>( SLayoutCounter++ );
        return VK_SUCCESS;
    }

    void vkDestroyDescriptorSetLayout ( VkDevice InDevice, VkDescriptorSetLayout InDescriptorSetLayout, const VkAllocationCallbacks *InAllocator )
    {
        ( void )InDevice;
        ( void )InDescriptorSetLayout;
        ( void )InAllocator;
    }

    VkResult vkCreateDescriptorPool ( VkDevice InDevice,
                                      const VkDescriptorPoolCreateInfo *InCreateInfo,
                                      const VkAllocationCallbacks *InAllocator,
                                      VkDescriptorPool *OutDescriptorPool )
    {
        ( void )InDevice;
        ( void )InCreateInfo;
        ( void )InAllocator;

        *OutDescriptorPool = reinterpret_cast<VkDescriptorPool>( 0x2222ULL );
        return VK_SUCCESS;
    }

    void vkDestroyDescriptorPool ( VkDevice InDevice, VkDescriptorPool InDescriptorPool, const VkAllocationCallbacks *InAllocator )
    {
        ( void )InDevice;
        ( void )InDescriptorPool;
        ( void )InAllocator;
    }

    VkResult vkAllocateDescriptorSets ( VkDevice InDevice, const VkDescriptorSetAllocateInfo *InAllocateInfo, VkDescriptorSet *OutDescriptorSets )
    {
        ( void )InDevice;

        static uintptr_t SSetCounter = 0x3333ULL;
        for ( uint32_t Index = 0U; Index < InAllocateInfo->descriptorSetCount; ++Index )
        {
            OutDescriptorSets[Index] = reinterpret_cast<VkDescriptorSet>( SSetCounter++ );
        }
        return VK_SUCCESS;
    }

    void vkUpdateDescriptorSets ( VkDevice InDevice,
                                  uint32_t InDescriptorWriteCount,
                                  const VkWriteDescriptorSet *InDescriptorWrites,
                                  uint32_t InDescriptorCopyCount,
                                  const VkCopyDescriptorSet *InDescriptorCopies )
    {
        ( void )InDevice;
        ( void )InDescriptorWriteCount;
        ( void )InDescriptorWrites;
        ( void )InDescriptorCopyCount;
        ( void )InDescriptorCopies;
    }
}

/**
 * @class FVulkanMemoryTest
 * @brief GTest fixture that resets mock allocator state before every test.
 */
class FVulkanMemoryTest : public ::testing::Test
{
protected:

    void SetUp () override
    {
        ResetMockState();
    }
};

/**
 * @brief Test 1: Config with valid MaxFramesInFlight initializes vectors to correct size.
 */
TEST_F( FVulkanMemoryTest, InitializeValidFramesInFlight )
{
    LumenEngine::VulkanRHI::FVulkanMemory Memory;
    LumenEngine::VulkanRHI::FDescriptorConfig Config;
    Config.MaxFramesInFlight = 2U;

    VkInstance DummyInstance         = reinterpret_cast<VkInstance>( 0xDEADBEEF1111ULL );
    VkPhysicalDevice DummyPhysDevice = reinterpret_cast<VkPhysicalDevice>( 0xDEADBEEF2222ULL );
    VkDevice DummyDevice             = reinterpret_cast<VkDevice>( 0xDEADBEEF3333ULL );

    Memory.Initialize( DummyInstance, DummyPhysDevice, DummyDevice, Config );

    EXPECT_EQ( Memory.GetNumFramesInFlight(), 2U );
    EXPECT_NE( Memory.GetAllocator(), VK_NULL_HANDLE );
    EXPECT_NE( Memory.GetDescriptorPool(), VK_NULL_HANDLE );

    EXPECT_NE( Memory.GetGlobalSetLayout(), VK_NULL_HANDLE );
    EXPECT_NE( Memory.GetSceneSetLayout(), VK_NULL_HANDLE );
    EXPECT_NE( Memory.GetCullSetLayout(), VK_NULL_HANDLE );

    EXPECT_NE( Memory.GetGlobalDescriptorSet( 0U ), VK_NULL_HANDLE );
    EXPECT_NE( Memory.GetGlobalDescriptorSet( 1U ), VK_NULL_HANDLE );

    Memory.Shutdown( DummyDevice );
}

/**
 * @brief Test 2: GetNumFramesInFlight returns the correct count.
 */
TEST_F( FVulkanMemoryTest, GetNumFramesInFlightReturnsCorrectCount )
{
    LumenEngine::VulkanRHI::FVulkanMemory Memory;
    LumenEngine::VulkanRHI::FDescriptorConfig Config;
    Config.MaxFramesInFlight = 3U;

    VkDevice DummyDevice = reinterpret_cast<VkDevice>( 0xDEADBEEF3333ULL );

    Memory.Initialize( VK_NULL_HANDLE, VK_NULL_HANDLE, DummyDevice, Config );

    EXPECT_EQ( Memory.GetNumFramesInFlight(), 3U );

    VkDescriptorSet Set0 = Memory.GetGlobalDescriptorSet( 0U );
    VkDescriptorSet Set1 = Memory.GetGlobalDescriptorSet( 1U );
    VkDescriptorSet Set2 = Memory.GetGlobalDescriptorSet( 2U );

    EXPECT_NE( Set0, VK_NULL_HANDLE );
    EXPECT_NE( Set1, VK_NULL_HANDLE );
    EXPECT_NE( Set2, VK_NULL_HANDLE );

    EXPECT_NE( Set0, Set1 );
    EXPECT_NE( Set1, Set2 );

    Memory.Shutdown( DummyDevice );
}

/**
 * @brief Test 3: FDescriptorConfig with MaxFramesInFlight = 0 initializes gracefully.
 */
TEST_F( FVulkanMemoryTest, InitializeZeroFramesInFlight )
{
    LumenEngine::VulkanRHI::FVulkanMemory Memory;
    LumenEngine::VulkanRHI::FDescriptorConfig Config;
    Config.MaxFramesInFlight = 0U;

    VkDevice DummyDevice = reinterpret_cast<VkDevice>( 0xDEADBEEF3333ULL );

    Memory.Initialize( VK_NULL_HANDLE, VK_NULL_HANDLE, DummyDevice, Config );

    EXPECT_EQ( Memory.GetNumFramesInFlight(), 0U );
    EXPECT_NE( Memory.GetAllocator(), VK_NULL_HANDLE );
    EXPECT_EQ( Memory.GetDescriptorPool(), VK_NULL_HANDLE );

    EXPECT_EQ( Memory.GetGlobalDescriptorSet( 0U ), VK_NULL_HANDLE );

    LumenEngine::VulkanRHI::FGPUGlobalUniforms Uniforms;
    Uniforms.TimeSeconds = 42.F;
    Memory.UpdateGlobalUniformData( 0U, Uniforms );

    Memory.Shutdown( DummyDevice );
}

/**
 * @brief Test 4: Bounds checking works for GetGlobalDescriptorSet and UpdateGlobalUniformData.
 */
TEST_F( FVulkanMemoryTest, BoundsCheckingForGetAndUpdate )
{
    LumenEngine::VulkanRHI::FVulkanMemory Memory;
    LumenEngine::VulkanRHI::FDescriptorConfig Config;
    Config.MaxFramesInFlight = 2U;

    VkDevice DummyDevice = reinterpret_cast<VkDevice>( 0xDEADBEEF3333ULL );

    Memory.Initialize( VK_NULL_HANDLE, VK_NULL_HANDLE, DummyDevice, Config );

    LumenEngine::VulkanRHI::FGPUGlobalUniforms UniformsFrame0;
    UniformsFrame0.TimeSeconds = 10.5F;
    UniformsFrame0.DeltaTime   = 0.016F;

    LumenEngine::VulkanRHI::FGPUGlobalUniforms UniformsFrame1;
    UniformsFrame1.TimeSeconds = 11.0F;
    UniformsFrame1.DeltaTime   = 0.033F;

    Memory.UpdateGlobalUniformData( 0U, UniformsFrame0 );
    Memory.UpdateGlobalUniformData( 1U, UniformsFrame1 );

    const LumenEngine::VulkanRHI::FGPUGlobalUniforms *CopiedFrame0 = reinterpret_cast<const LumenEngine::VulkanRHI::FGPUGlobalUniforms *>( GMockMappedMemory[0U] );
    const LumenEngine::VulkanRHI::FGPUGlobalUniforms *CopiedFrame1 = reinterpret_cast<const LumenEngine::VulkanRHI::FGPUGlobalUniforms *>( GMockMappedMemory[1U] );

    EXPECT_FLOAT_EQ( CopiedFrame0->TimeSeconds, 10.5F );
    EXPECT_FLOAT_EQ( CopiedFrame0->DeltaTime, 0.016F );
    EXPECT_FLOAT_EQ( CopiedFrame1->TimeSeconds, 11.0F );
    EXPECT_FLOAT_EQ( CopiedFrame1->DeltaTime, 0.033F );

    EXPECT_EQ( Memory.GetGlobalDescriptorSet( 2U ), VK_NULL_HANDLE );
    EXPECT_EQ( Memory.GetGlobalDescriptorSet( 999U ), VK_NULL_HANDLE );

    LumenEngine::VulkanRHI::FGPUGlobalUniforms BadUniforms;
    BadUniforms.TimeSeconds = 999.F;

    const LumenEngine::VulkanRHI::FGPUGlobalUniforms *CopiedFrame2 = reinterpret_cast<const LumenEngine::VulkanRHI::FGPUGlobalUniforms *>( GMockMappedMemory[2U] );
    const LumenEngine::VulkanRHI::FGPUGlobalUniforms *CopiedFrame3 = reinterpret_cast<const LumenEngine::VulkanRHI::FGPUGlobalUniforms *>( GMockMappedMemory[3U] );

    Memory.UpdateGlobalUniformData( 2U, BadUniforms );
    Memory.UpdateGlobalUniformData( 999U, BadUniforms );

    EXPECT_FLOAT_EQ( CopiedFrame2->TimeSeconds, 0.0F );
    EXPECT_FLOAT_EQ( CopiedFrame3->TimeSeconds, 0.0F );

    Memory.Shutdown( DummyDevice );
}
