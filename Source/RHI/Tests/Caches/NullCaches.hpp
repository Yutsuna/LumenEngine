/**
 * @file NullCaches.hpp
 * @brief Declares the deterministic versioned Null pipeline cache.
 */
#pragma once

#include <Caches/IRHIPipelineCache.hpp>
#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Templates/Map.hpp>
#include <Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace Tests
{

    /** @struct FNullPipelineCompatibilityHasher @brief Content hasher for complete pipeline keys. */
    struct FNullPipelineCompatibilityHasher
    {
        [[nodiscard]] USize operator()( const FRHIPipelineCompatibility &InKey ) const noexcept;
    };

    /** @class FNullPipelineCache @brief Thread-safe LRU pipeline cache with opaque metadata serialization. */
    class FNullPipelineCache final : public IRHIPipelineCache
    {
    public:

        FNullPipelineCache ( const FRHIPipelineCacheDesc &InDesc,
                             TSharedPtr<FNullFaultPlan> InFaultPlan,
                             FRHIObjectId InId ) noexcept;
        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Validates and restores the descriptor's initial opaque data. */
        [[nodiscard]] TExpected<void, FRHIError> Initialize () noexcept;
        const FRHIPipelineCacheDesc &GetDesc () const noexcept override;
        TSharedPtr<IRHIPipeline> Find ( const FRHIPipelineCompatibility &InKey ) noexcept override;
        TExpected<void, FRHIError> Insert ( const FRHIPipelineCompatibility &InKey,
                                            TSharedRef<IRHIPipeline> InPipeline ) noexcept override;
        void Remove ( const FRHIPipelineCompatibility &InKey ) noexcept override;
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
        TMap<FRHIPipelineCompatibility, FEntry, FNullPipelineCompatibilityHasher> Entries;
        UInt64 Sequence = 0ULL;
        FRHICacheStatsTracker Stats;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace Tests

} // namespace Lumen
