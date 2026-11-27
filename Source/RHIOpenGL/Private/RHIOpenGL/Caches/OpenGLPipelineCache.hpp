/**
 * @file OpenGLPipelineCache.hpp
 * @brief Declares the bounded OpenGL pipeline cache with versioned serialization.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <HAL/PlatformMutex.hpp>

#include <Memory/SharedPtr.hpp>

#include <RHI/Caches/IRHIPipelineCache.hpp>
#include <RHI/Caches/RHICacheStats.hpp>

#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Vector.hpp>

#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLPipelineCache
     * @brief Thread-safe LRU cache of immutable pipelines and opaque metadata.
     */
    class FOpenGLPipelineCache final : public RHI::IRHIPipelineCache, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLPipelineCache>, RHI::FRHIError>
        Create ( const RHI::FRHIPipelineCacheDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        FOpenGLPipelineCache ( const RHI::FRHIPipelineCacheDesc &InDesc, RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLPipelineCache () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Validates and restores the descriptor's initial opaque data. */
        [[nodiscard]] RHI::FRHIExpected Initialize () noexcept;

        [[nodiscard]] const RHI::FRHIPipelineCacheDesc &GetDesc () const noexcept override;

        [[nodiscard]] TOptional<TSharedPtr<RHI::IRHIPipeline>> Find ( const RHI::FRHIPipelineCacheKey &InKey ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Insert ( const RHI::FRHIPipelineCacheKey &InKey,
                                                 TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept override;

        void Remove ( const RHI::FRHIPipelineCacheKey &InKey ) noexcept override;

        void Clear () noexcept override;

        [[nodiscard]] RHI::FRHICacheStats GetStats () const noexcept override;

        [[nodiscard]] TExpected<TVector<Byte>, RHI::FRHIError> Serialize () const noexcept override;

    private:

        struct FEntry
        {
            TSharedPtr<RHI::IRHIPipeline> Pipeline;
            UInt64 Bytes    = 0ULL;
            UInt64 LastUsed = 0ULL;
        };

        struct FKeyHasher
        {
            [[nodiscard]] USize operator()( const RHI::FRHIPipelineCacheKey &InKey ) const noexcept;
        };

        [[nodiscard]] Bool MakeRoom ( UInt64 InBytes ) noexcept;
        void EvictOne () noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        RHI::FRHIPipelineCacheDesc Desc;
        TMap<RHI::FRHIPipelineCacheKey, FEntry, FKeyHasher> Entries;
        UInt64 Sequence = 0ULL;
        RHI::FRHICacheStatsTracker Stats;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHIOpenGL

} // namespace Lumen
