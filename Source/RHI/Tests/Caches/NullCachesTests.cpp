/**
 * @file NullCachesTests.cpp
 * @brief Tests Null pipeline cache hits, eviction, serialization, and corruption handling.
 */

#include <Caches/NullCaches.hpp>
#include <Pipelines/NullPipelines.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        FRHIPipelineCompatibility MakeKey ( const UInt64 InLayout ) noexcept
        {
            FRHIPipelineCompatibility Key;
            Key.LayoutHash       = InLayout;
            Key.ColorFormats     = { ERHIFormat::RGBA8Unorm };
            Key.VertexLayoutHash = InLayout + 1ULL;
            return Key;
        }

        TSharedRef<Tests::FNullPipeline> MakePipeline ( const FRHIPipelineCompatibility &InKey ) noexcept
        {
            return MakeShared<Tests::FNullPipeline>( ERHIPipelineType::Graphics, InKey, FRHIObjectId{ InKey.LayoutHash + 1ULL } );
        }

        LUMEN_TEST( NullCaches, FindsPipelinesAndEvictsLeastRecentlyUsedEntries )
        {
            FRHIPipelineCacheDesc Desc;
            Desc.MaxEntries = 2U;
            Tests::FNullPipelineCache Cache( Desc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 1ULL } );
            const FRHIPipelineCompatibility First  = MakeKey( 1ULL );
            const FRHIPipelineCompatibility Second = MakeKey( 2ULL );
            const FRHIPipelineCompatibility Third  = MakeKey( 3ULL );
            LUMEN_CHECK_TRUE( Cache.Insert( First, MakePipeline( First ) ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Insert( Second, MakePipeline( Second ) ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Find( First ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Insert( Third, MakePipeline( Third ) ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Find( First ).HasValue() );
            LUMEN_CHECK_FALSE( Cache.Find( Second ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Find( Third ).HasValue() );
            LUMEN_CHECK_EQ( Cache.GetStats().Evictions, 1ULL );
            LUMEN_CHECK_HAS_ERROR( Cache.Insert( MakeKey( 9ULL ), MakePipeline( First ) ) );
        }

        LUMEN_TEST( NullCaches, FindReportsHitAsValueAndMissAsEmpty )
        {
            Tests::FNullPipelineCache Cache( {}, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 7ULL } );
            const FRHIPipelineCompatibility Key = MakeKey( 8ULL );
            LUMEN_CHECK_FALSE( Cache.Find( Key ).HasValue() );
            LUMEN_CHECK_TRUE( Cache.Insert( Key, MakePipeline( Key ) ).HasValue() );
            const TOptional<TSharedPtr<IRHIPipeline>> Found = Cache.Find( Key );
            LUMEN_CHECK_TRUE( Found.HasValue() );
            LUMEN_CHECK_TRUE( ( *Found ).IsValid() );
        }

        LUMEN_TEST( NullCaches, SerializesDeterministicallyAndRestoresMetadata )
        {
            Tests::FNullPipelineCache Cache( {}, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 2ULL } );
            const FRHIPipelineCompatibility Key = MakeKey( 7ULL );
            LUMEN_CHECK_TRUE( Cache.Insert( Key, MakePipeline( Key ) ).HasValue() );
            TExpected<TVector<Byte>, FRHIError> Serialized = Cache.Serialize();
            LUMEN_CHECK_TRUE( Serialized.HasValue() );
            const TExpected<TVector<Byte>, FRHIError> Again = Cache.Serialize();
            LUMEN_CHECK_TRUE( Again.HasValue() );
            LUMEN_CHECK_TRUE( *Serialized == *Again );

            FRHIPipelineCacheDesc RestoredDesc;
            RestoredDesc.InitialData = *Serialized;
            Tests::FNullPipelineCache Restored( RestoredDesc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 3ULL } );
            LUMEN_CHECK_TRUE( Restored.Initialize().HasValue() );
            LUMEN_CHECK_EQ( Restored.GetStats().EntryCount, 1ULL );
            LUMEN_CHECK_FALSE( Restored.Find( Key ).HasValue() );
            LUMEN_CHECK_TRUE( Restored.Insert( Key, MakePipeline( Key ) ).HasValue() );
            LUMEN_CHECK_TRUE( Restored.Find( Key ).HasValue() );
            const TExpected<TVector<Byte>, FRHIError> RoundTrip = Restored.Serialize();
            LUMEN_CHECK_TRUE( RoundTrip.HasValue() );
            LUMEN_CHECK_TRUE( *RoundTrip == *Serialized );
        }

        LUMEN_TEST( NullCaches, RejectsCorruptionAndCapacityOverflow )
        {
            Tests::FNullPipelineCache Cache( {}, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 4ULL } );
            const FRHIPipelineCompatibility Key = MakeKey( 4ULL );
            LUMEN_CHECK_TRUE( Cache.Insert( Key, MakePipeline( Key ) ).HasValue() );
            TVector<Byte> Data = *Cache.Serialize();
            Data[0]            = static_cast<Byte>( 'X' );
            FRHIPipelineCacheDesc CorruptedDesc;
            CorruptedDesc.InitialData = Data;
            Tests::FNullPipelineCache Corrupted( CorruptedDesc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 5ULL } );
            LUMEN_CHECK_HAS_ERROR( Corrupted.Initialize() );

            FRHIPipelineCacheDesc BoundedDesc;
            BoundedDesc.MaxEntries     = 1U;
            BoundedDesc.bAllowEviction = false;
            Tests::FNullPipelineCache Bounded( BoundedDesc, MakeShared<Tests::FNullFaultPlan>(), FRHIObjectId{ 6ULL } );
            LUMEN_CHECK_TRUE( Bounded.Insert( Key, MakePipeline( Key ) ).HasValue() );
            const FRHIPipelineCompatibility Other = MakeKey( 5ULL );
            LUMEN_CHECK_HAS_ERROR( Bounded.Insert( Other, MakePipeline( Other ) ) );
            LUMEN_CHECK_TRUE( Bounded.Find( Key ).HasValue() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
