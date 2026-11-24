/**
 * @file RHIPipelineCacheKey.hpp
 * @brief Defines complete content-addressed pipeline cache keys.
 */
#pragma once

#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>
#include <RHI/Pipelines/RHIPipelineCompatibility.hpp>

namespace Lumen
{

namespace RHI
{
    /**
     * @struct FRHIPipelineCacheKey
     * @brief Compatibility, immutable state, ordered shaders, and backend schema identity.
     */
    struct RHI_EXPORT FRHIPipelineCacheKey
    {
        FRHIPipelineCompatibility Compatibility;
        UInt64 StateHash = 0ULL;
        TVector<UInt64> ShaderHashes;
        UInt64 BackendCacheSchemaVersion = 1ULL;

        /** @return Stable hash of every key field in declaration order. */
        [[nodiscard]] UInt64 GetHash () const noexcept;
        /** @return Equality of all cache identity fields. */
        [[nodiscard]] Bool operator==( const FRHIPipelineCacheKey & ) const noexcept = default;
    };

    /** @return Content and reflection hash of one shader, excluding object identity and debug names. */
    [[nodiscard]] RHI_EXPORT UInt64 GetRHIShaderCacheHash ( const IRHIShader &InShader ) noexcept;

    /** @return Complete graphics key; InDesc is expected to satisfy Validate(). */
    [[nodiscard]] RHI_EXPORT FRHIPipelineCacheKey MakeRHIGraphicsPipelineCacheKey ( const FRHIGraphicsPipelineDesc &InDesc,
                                                                                    UInt64 InBackendCacheSchemaVersion ) noexcept;

    /** @return Complete compute key; InDesc is expected to satisfy Validate(). */
    [[nodiscard]] RHI_EXPORT FRHIPipelineCacheKey MakeRHIComputePipelineCacheKey ( const FRHIComputePipelineDesc &InDesc,
                                                                                   UInt64 InBackendCacheSchemaVersion ) noexcept;
} // namespace RHI

} // namespace Lumen
