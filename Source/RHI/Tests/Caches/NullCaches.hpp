/**
 * @file NullCaches.hpp
 * @brief Declares the deterministic versioned Null pipeline cache.
 */
#pragma once

#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <RHI/Caches/IRHIPipelineCache.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/Map.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @struct FNullPipelineCacheKeyHasher
         * @brief Content hasher for complete pipeline keys.
         */
        struct FNullPipelineCacheKeyHasher
        {
            [[nodiscard]] USize operator()( const FRHIPipelineCacheKey &InKey ) const noexcept;
        };

        /**
         * @class FNullPipelineCache
         * @brief Thread-safe LRU pipeline cache with opaque metadata serialization.
         */
        class FNullPipelineCache final : public IRHIPipelineCache
        {
        public:

            FNullPipelineCache ( const FRHIPipelineCacheDesc &InDesc,
                                 TSharedPtr<FNullFaultPlan> InFaultPlan,
                                 FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;

            /** @brief Validates and restores the descriptor's initial opaque data. */
            [[nodiscard]] FRHIExpected Initialize () noexcept;
            const FRHIPipelineCacheDesc &GetDesc () const noexcept override;
            TOptional<TSharedPtr<IRHIPipeline>> Find ( const FRHIPipelineCacheKey &InKey ) noexcept override;
            FRHIExpected Insert ( const FRHIPipelineCacheKey &InKey, TSharedRef<IRHIPipeline> InPipeline ) noexcept override;
            void Remove ( const FRHIPipelineCacheKey &InKey ) noexcept override;
            void Clear () noexcept override;
            FRHICacheStats GetStats () const noexcept override;
            TExpected<TVector<Byte>, FRHIError> Serialize () const noexcept override;

        private:

            struct FEntry
            {
                TSharedPtr<IRHIPipeline> Pipeline;
                UInt64 Bytes    = 0ULL;
                UInt64 LastUsed = 0ULL;
            };

            [[nodiscard]] Bool MakeRoom ( UInt64 InBytes ) noexcept;
            void EvictOne () noexcept;

        private:

            FRHIObjectBase Identity;
            FRHIPipelineCacheDesc Desc;
            TSharedPtr<FNullFaultPlan> FaultPlan;
            TMap<FRHIPipelineCacheKey, FEntry, FNullPipelineCacheKeyHasher> Entries;
            UInt64 Sequence = 0ULL;
            FRHICacheStatsTracker Stats;
            mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
