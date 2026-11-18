/**
 * @file RHIObjectCacheStressTests.cpp
 * @brief Stresses concurrent object cache lookup, creation, and eviction.
 */

#include <Caches/RHIObjectCache.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

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

    LUMEN_TEST( RHIObjectCacheStress, ConcurrentGetOrCreateKeepsCanonicalValues )
    {
        TRHIObjectCache<UInt32, FCachedValue> Cache( 8U );
        TAtomic<UInt32> Failures{ 0U };
        TVector<FThread> Workers;
        for ( UInt32 Worker = 0U; Worker < 4U; ++Worker )
        {
            Workers.emplace_back( [&Cache, &Failures, Worker]
            {
                for ( UInt32 Index = 0U; Index < 128U; ++Index )
                {
                    const UInt32 Key                                            = ( Index + Worker ) % 16U;
                    const TExpected<TSharedPtr<FCachedValue>, FRHIError> Result = Cache.GetOrCreate( Key, [Key]
                    {
                        return TExpected<TSharedRef<FCachedValue>, FRHIError>( MakeShared<FCachedValue>( Key ) );
                    }, 8ULL );
                    if ( not Result or ( *Result )->Value != Key )
                    {
                        Failures.fetch_add( 1U );
                    }
                }
            } );
        }
        for ( auto &Worker : Workers )
        {
            Worker.Join();
        }
        const FRHICacheStats Stats = Cache.GetStats();
        LUMEN_CHECK_EQ( Failures.load(), 0U );
        LUMEN_CHECK_EQ( Stats.Hits + Stats.Misses, 512ULL );
        LUMEN_CHECK_TRUE( Stats.EntryCount <= 8ULL );
        LUMEN_CHECK_EQ( Stats.Bytes, Stats.EntryCount * 8ULL );
    }

} // namespace
} // namespace Lumen
