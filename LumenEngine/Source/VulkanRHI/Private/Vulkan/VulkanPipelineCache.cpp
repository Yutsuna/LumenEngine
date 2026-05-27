/**
 * @file VulkanPipelineCache.cpp
 * @brief Implementation of the FVulkanPipelineCache class.
 */

#include "Vulkan/VulkanPipelineCache.hpp"

#include "Filesystem/Directory.hpp"
#include "Filesystem/File.hpp"
#include "Logging/Logger.hpp"
#include "Vulkan/VulkanCore.hpp"

#include <utility>

/**
 * Movable
 */

LumenEngine::VulkanRHI::FVulkanPipelineCache::FVulkanPipelineCache ( FVulkanPipelineCache &&InOther ) noexcept
    : CacheHandle( InOther.CacheHandle ), CacheFilePath( std::move( InOther.CacheFilePath ) )
{
    InOther.CacheHandle = VK_NULL_HANDLE;
}

LumenEngine::VulkanRHI::FVulkanPipelineCache &LumenEngine::VulkanRHI::FVulkanPipelineCache::operator=( FVulkanPipelineCache &&InOther ) noexcept
{
    if ( this != &InOther )
    {
        CacheHandle         = InOther.CacheHandle;
        CacheFilePath       = std::move( InOther.CacheFilePath );
        InOther.CacheHandle = VK_NULL_HANDLE;
    }
    return *this;
}

/**
 * Public
 */

void LumenEngine::VulkanRHI::FVulkanPipelineCache::Initialize ( VkDevice InDevice, const Filesystem::FPath &InCachePath ) noexcept
{
    CacheFilePath = InCachePath;

    TVector<Byte> CacheData;
    VkPipelineCacheCreateInfo CacheCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, .pNext = nullptr, .flags = 0, .initialDataSize = 0, .pInitialData = nullptr };

    if ( Filesystem::FFile::Exists( CacheFilePath ) )
    {
        TExpected<TVector<Byte>, EErrorCode::Type> LoadResult = Filesystem::FFile::ReadAllBytes( CacheFilePath );

        if ( not LoadResult.has_value() )
        {
            LUMEN_LOG_WARNING( LogVulkanRHI, "Failed to read pipeline cache file: {}", CacheFilePath.ToString().c_str() );
        }
        else
        {
            CacheData                       = std::move( *LoadResult );
            CacheCreateInfo.initialDataSize = CacheData.size();
            CacheCreateInfo.pInitialData    = CacheData.data();
            LUMEN_LOG_INFO( LogVulkanRHI, "Loaded pipeline cache from disk: {} ({} bytes)", CacheFilePath.ToString().c_str(), CacheData.size() );
        }
    }
    else
    {
        LUMEN_LOG_INFO( LogVulkanRHI, "No existing pipeline cache file found at: {}", CacheFilePath.ToString().c_str() );
    }
    LUMEN_VK_CHECK( vkCreatePipelineCache( InDevice, &CacheCreateInfo, nullptr, &CacheHandle ) );
}

void LumenEngine::VulkanRHI::FVulkanPipelineCache::SaveToDisk ( VkDevice InDevice ) noexcept
{
    if ( CacheHandle == VK_NULL_HANDLE )
    {
        return;
    }

    USize CacheSize = 0;
    VkResult Result = vkGetPipelineCacheData( InDevice, CacheHandle, &CacheSize, nullptr );
    if ( Result != VK_SUCCESS or CacheSize == 0 )
    {
        LUMEN_LOG_WARNING( LogVulkanRHI, "Failed to retrieve Vulkan pipeline cache data size." );
        return;
    }

    TVector<Byte> CacheBuffer( CacheSize );
    Result = vkGetPipelineCacheData( InDevice, CacheHandle, &CacheSize, CacheBuffer.data() );
    if ( Result != VK_SUCCESS )
    {
        LUMEN_LOG_WARNING( LogVulkanRHI, "Failed to retrieve Vulkan pipeline cache data." );
        return;
    }

    const Filesystem::FPath ParentPath = CacheFilePath.GetParentPath();
    if ( not Filesystem::FDirectory::Exists( ParentPath ) )
    {
        TExpected<void, EErrorCode::Type> CreateResult = Filesystem::FDirectory::CreateDirectories( ParentPath );
        if ( not CreateResult.has_value() )
        {
            LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to create directory for pipeline cache: {}", ParentPath.ToString().c_str() );
            return;
        }
    }

    TExpected<void, EErrorCode::Type> WriteResult = Filesystem::FFile::WriteAllBytes<Byte>( CacheFilePath, CacheBuffer );
    if ( WriteResult.has_value() )
    {
        LUMEN_LOG_INFO( LogVulkanRHI, "Saved pipeline cache to: {}, size: {} bytes", CacheFilePath.ToString().c_str(), CacheSize );
    }
    else
    {
        LUMEN_LOG_ERROR( LogVulkanRHI, "Failed to write pipeline cache data to disk: {}", CacheFilePath.ToString().c_str() );
    }
}

void LumenEngine::VulkanRHI::FVulkanPipelineCache::Shutdown ( VkDevice InDevice ) noexcept
{
    if ( CacheHandle != VK_NULL_HANDLE )
    {
        vkDestroyPipelineCache( InDevice, CacheHandle, nullptr );
        CacheHandle = VK_NULL_HANDLE;
    }
}

VkPipelineCache LumenEngine::VulkanRHI::FVulkanPipelineCache::GetHandle () const noexcept
{
    return CacheHandle;
}
