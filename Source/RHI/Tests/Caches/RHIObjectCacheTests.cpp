/**
 * @file RHIObjectCacheTests.cpp
 * @brief Tests object cache reuse, factory failures, removal, and LRU eviction.
 */

#include <Caches/RHIObjectCache.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    struct FCachedValue
    {
        explicit FCachedValue ( const UInt32 InValue ) noexcept : Value( InValue )
        {
        }
        UInt32 Value = 0U;
    };

    using FCache = TRHIObjectCache<UInt32, FCachedValue>;

    LUMEN_TEST( RHIObjectCache, ReusesValuesAndCountsHitsAndMisses )
    {
        FCache Cache( 4U );
        const auto Factory = []
        {
            return TExpected<TSharedRef<FCachedValue>, FRHIError>( MakeShared<FCachedValue>( 7U ) );
        };
        auto First = Cache.GetOrCreate( 7U, Factory, 16ULL );
        LUMEN_CHECK_TRUE( First.HasValue() );
        auto Second = Cache.GetOrCreate( 7U, Factory, 16ULL );
        LUMEN_CHECK_TRUE( Second.HasValue() );
        LUMEN_CHECK_TRUE( *First == *Second );
        LUMEN_CHECK_EQ( ( *First )->Value, 7U );
        const auto Stats = Cache.GetStats();
        LUMEN_CHECK_EQ( Stats.Hits, 1ULL );
        LUMEN_CHECK_EQ( Stats.Misses, 1ULL );
        LUMEN_CHECK_EQ( Stats.EntryCount, 1ULL );
        Cache.Remove( 7U );
        LUMEN_CHECK_EQ( Cache.GetEntryCount(), 0U );
    }

    LUMEN_TEST( RHIObjectCache, EvictsTheLeastRecentlyUsedEntry )
    {
        FCache Cache( 2U );
        const auto Factory = [] ( const UInt32 Key )
        {
            return TExpected<TSharedRef<FCachedValue>, FRHIError>( MakeShared<FCachedValue>( Key ) );
        };
        LUMEN_CHECK_TRUE( Cache.Store( 1U, MakeShared<FCachedValue>( 1U ), 8ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Cache.Store( 2U, MakeShared<FCachedValue>( 2U ), 8ULL ).HasValue() );
        LUMEN_CHECK_TRUE( Cache.Find( 1U ).IsValid() );
        auto Third = Cache.GetOrCreate( 3U, [&Factory]
        {
            return Factory( 3U );
        }, 8ULL );
        LUMEN_CHECK_TRUE( Third.HasValue() );
        LUMEN_CHECK_TRUE( Cache.Find( 1U ).IsValid() );
        LUMEN_CHECK_FALSE( Cache.Find( 2U ).IsValid() );
        LUMEN_CHECK_TRUE( Cache.Find( 3U ).IsValid() );
        LUMEN_CHECK_EQ( Cache.GetStats().Evictions, 1ULL );
        Cache.Clear();
        LUMEN_CHECK_EQ( Cache.GetEntryCount(), 0U );
        LUMEN_CHECK_EQ( Cache.GetStats().Hits, 0ULL );
    }

    LUMEN_TEST( RHIObjectCache, DoesNotInsertFailedOrRejectedCreations )
    {
        FCache Cache( 1U, ERHICacheEvictionPolicy::None );
        const auto Failure = Cache.GetOrCreate( 9U, []
        {
            return TExpected<TSharedRef<FCachedValue>, FRHIError>(
                TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Cache, "factory" ) ) );
        }, 8ULL );
        LUMEN_CHECK_HAS_ERROR( Failure );
        LUMEN_CHECK_EQ( Cache.GetEntryCount(), 0U );
        LUMEN_CHECK_TRUE( Cache.Store( 1U, MakeShared<FCachedValue>( 1U ), 8ULL ).HasValue() );
        auto Rejected = Cache.GetOrCreate( 2U, []
        {
            return TExpected<TSharedRef<FCachedValue>, FRHIError>( MakeShared<FCachedValue>( 2U ) );
        }, 8ULL );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Cache.GetEntryCount(), 1U );
        LUMEN_CHECK_TRUE( Cache.Find( 1U ).IsValid() );
    }

} // namespace
} // namespace Lumen
