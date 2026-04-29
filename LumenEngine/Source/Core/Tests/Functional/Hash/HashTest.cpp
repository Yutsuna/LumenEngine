/**
 * @file HashTest.cpp
 * @brief Unit tests for the Core Hash module.
 */

#include "Hash/Hash.hpp"

#include "Container/Map.hpp"
#include "Container/String.hpp"
#include "Container/Vector.hpp"
#include "CoreTypes.hpp"

#include <gtest/gtest.h>

#include <thread>

namespace LumenEngine
{

namespace Hash
{

    static_assert( Concepts::CHashAlgorithm<FFnv1a64>, "FFnv1a64 must satisfy CHashAlgorithm" );
    static_assert( Concepts::CHashable<Int32>, "Int32 must be CHashable" );
    static_assert( Concepts::CHashable<UInt64>, "UInt64 must be CHashable" );
    static_assert( Concepts::CHashable<Float32>, "Float32 must be CHashable" );
    static_assert( Concepts::CHashable<Float64>, "Float64 must be CHashable" );
    static_assert( Concepts::CHashable<Bool>, "Bool must be CHashable" );
    static_assert( Concepts::CHashable<FString>, "FString must be CHashable" );
    static_assert( Concepts::CHashable<FStringView>, "FStringView must be CHashable" );
    static_assert( Concepts::CCacheKey<Int32>, "Int32 must be CCacheKey" );
    static_assert( Concepts::CCacheKey<UInt64>, "UInt64 must be CCacheKey" );
    static_assert( Concepts::CCacheKey<FString>, "FString must be CCacheKey" );

    /**
     * FFnv1a64
     */

    TEST( FFnv1a64, DefaultStateIsOffsetBasis )
    {
        FFnv1a64 Algo;
        EXPECT_EQ( Algo.Digest(), FFnv1a64::OffsetBasis );
    }

    TEST( FFnv1a64, DeterministicForSameInput )
    {
        FFnv1a64 A;
        FFnv1a64 B;
        const AnsiChar Data[] = "hello world";

        A.Write( Data, sizeof( Data ) - 1 );
        B.Write( Data, sizeof( Data ) - 1 );
        EXPECT_EQ( A.Digest(), B.Digest() );
    }

    TEST( FFnv1a64, DifferentInputsProduceDifferentHashes )
    {
        FFnv1a64 A;
        FFnv1a64 B;
        A.Write( "foo", 3 );
        B.Write( "bar", 3 );
        EXPECT_NE( A.Digest(), B.Digest() );
    }

    TEST( FFnv1a64, IncrementalEqualsOneShot )
    {
        const AnsiChar Part1[] = "hello";
        const AnsiChar Part2[] = "world";

        FFnv1a64 Incremental;
        Incremental.Write( Part1, 5 );
        Incremental.Write( Part2, 5 );

        FFnv1a64 OneShot;
        OneShot.Write( "helloworld", 10 );

        EXPECT_EQ( Incremental.Digest(), OneShot.Digest() );
    }

    TEST( FFnv1a64, ResetRestoresOffsetBasis )
    {
        FFnv1a64 Algo;
        Algo.Write( "data", 4 );
        const FHashValue Before = Algo.Digest();

        Algo.Reset();
        EXPECT_EQ( Algo.Digest(), FFnv1a64::OffsetBasis );
        EXPECT_NE( Algo.Digest(), Before );
    }

    TEST( FFnv1a64, CustomSeed )
    {
        constexpr FHashValue Seed = 0xDEADBEEFCAFEBABEULL;
        FFnv1a64 Algo( Seed );
        Algo.Write( "x", 1 );

        FFnv1a64 DefaultAlgo;
        DefaultAlgo.Write( "x", 1 );

        EXPECT_NE( Algo.Digest(), DefaultAlgo.Digest() );
    }

    TEST( FFnv1a64, Constexpr )
    {
        constexpr FHashValue Hash = [] () constexpr
        {
            FFnv1a64 Algo;
            const AnsiChar Data[] = "constexpr";
            Algo.Write( Data, 9 );
            return Algo.Digest();
        }();

        EXPECT_NE( Hash, FFnv1a64::OffsetBasis );
    }

    /**
     * THasher - integer types
     */

    TEST( THasher, IntegersDifferentValues )
    {
        THasher<Int32> H;
        EXPECT_NE( H( 0 ), H( 1 ) );
        EXPECT_NE( H( -1 ), H( 1 ) );
        EXPECT_NE( H( std::numeric_limits<Int32>::min() ), H( std::numeric_limits<Int32>::max() ) );
    }

    TEST( THasher, UInt64Deterministic )
    {
        THasher<UInt64> H;
        const UInt64 Val = 0xCAFEBABEDEADBEEFULL;
        EXPECT_EQ( H( Val ), H( Val ) );
    }

    TEST( THasher, ZeroIsHashable )
    {
        EXPECT_NO_THROW( ( void )THasher<Int64>{}( 0LL ) );
        EXPECT_NO_THROW( ( void )THasher<UInt64>{}( 0ULL ) );
    }

    /**
     * THasher - floating-point
     */

    TEST( THasher, PositiveZeroEqualsNegativeZero_Float32 )
    {
        THasher<Float32> H;
        EXPECT_EQ( H( 0.0F ), H( -0.0F ) );
    }

    TEST( THasher, PositiveZeroEqualsNegativeZero_Float64 )
    {
        THasher<Float64> H;
        EXPECT_EQ( H( 0.0 ), H( -0.0 ) );
    }

    TEST( THasher, DifferentFloatsDifferentHashes )
    {
        THasher<Float32> H;
        EXPECT_NE( H( 1.0F ), H( 2.0F ) );
    }

    /**
     * THasher - Bool
     */

    TEST( THasher, BoolTrueAndFalseDiffer )
    {
        THasher<Bool> H;
        EXPECT_NE( H( true ), H( false ) );
    }

    /**
     * THasher - FString / FStringView
     */

    TEST( THasher, FStringDeterministic )
    {
        THasher<FString> H;
        EXPECT_EQ( H( FString( "hello" ) ), H( FString( "hello" ) ) );
    }

    TEST( THasher, FStringDifferentValues )
    {
        THasher<FString> H;
        EXPECT_NE( H( FString( "foo" ) ), H( FString( "bar" ) ) );
    }

    TEST( THasher, FStringViewMatchesFString )
    {
        const FString Str( "lumen" );
        const FStringView View( Str );
        EXPECT_EQ( THasher<FString>{}( Str ), THasher<FStringView>{}( View ) );
    }

    TEST( THasher, EmptyStringIsHashable )
    {
        EXPECT_NO_THROW( ( void )THasher<FString>{}( FString{} ) );
    }

    /**
     * THasher - pointers
     */

    TEST( THasher, NullPointerIsHashable )
    {
        EXPECT_NO_THROW( ( void )THasher<Int32 *>{}( nullptr ) );
    }

    TEST( THasher, DifferentPointersProduceDifferentHashes )
    {
        Int32 A = 1;
        Int32 B = 2;

        THasher<Int32 *> H;
        EXPECT_NE( H( &A ), H( &B ) );
    }

    /**
     * THasher - std::pair
     */

    TEST( THasher, PairDeterministic )
    {
        using P = std::pair<Int32, Int32>;
        THasher<P> H;
        EXPECT_EQ( H( { 1, 2 } ), H( { 1, 2 } ) );
    }

    TEST( THasher, PairOrderMatters )
    {
        using P = std::pair<Int32, Int32>;
        THasher<P> H;
        EXPECT_NE( H( { 1, 2 } ), H( { 2, 1 } ) );
    }

    /**
     * THasher - std::tuple
     */

    TEST( THasher, TupleDeterministic )
    {
        using T = std::tuple<Int32, UInt64, Bool>;
        THasher<T> H;
        EXPECT_EQ( H( { 1, 2ULL, true } ), H( { 1, 2ULL, true } ) );
    }

    TEST( THasher, TupleDifferentElements )
    {
        using T = std::tuple<Int32, Int32>;
        THasher<T> H;
        EXPECT_NE( H( { 1, 2 } ), H( { 1, 3 } ) );
    }

    /**
     * Compute / ComputeRaw / CombineHash
     */

    TEST( HashFunctions, ComputeMatchesTHasher )
    {
        const Int32 Val = 42;
        EXPECT_EQ( Compute( Val ), THasher<Int32>{}( Val ) );
    }

    TEST( HashFunctions, ComputeRawMatchesFFnv1a64 )
    {
        const AnsiChar Data[] = "raw bytes";
        FFnv1a64 Algo;
        Algo.Write( Data, sizeof( Data ) - 1 );

        EXPECT_EQ( ComputeRaw( Data, sizeof( Data ) - 1 ), Algo.Digest() );
    }

    TEST( HashFunctions, CombineHashDifferentSeeds )
    {
        const Int32 Val     = 99;
        const FHashValue H1 = CombineHash( 0ULL, Val );
        const FHashValue H2 = CombineHash( 1ULL, Val );
        EXPECT_NE( H1, H2 );
    }

    TEST( HashFunctions, CombineHashAssociative_NotCommutative )
    {
        const FHashValue Base = FFnv1a64::OffsetBasis;
        const FHashValue AB   = CombineHash( CombineHash( Base, 1 ), 2 );
        const FHashValue BA   = CombineHash( CombineHash( Base, 2 ), 1 );
        EXPECT_NE( AB, BA );
    }

    /**
     * TStdHashAdapter - unordered_map integration
     */

    TEST( TStdHashAdapter, IntKeyWorksInUnorderedMap )
    {
        TMap<Int32, FString, TStdHashAdapter<Int32>> Map;
        Map.emplace( 1, "one" );
        Map.emplace( 2, "two" );

        ASSERT_TRUE( Map.contains( 1 ) );
        EXPECT_EQ( Map.at( 1 ), "one" );
        EXPECT_EQ( Map.size(), 2U );
    }

    TEST( TStdHashAdapter, FStringTransparentLookup )
    {
        TMap<FString, Int32, TStdHashAdapter<FString>, std::equal_to<>> Map;
        Map.emplace( "alpha", 1 );

        const FStringView Key = "alpha";
        ASSERT_TRUE( Map.contains( Key ) );
        EXPECT_EQ( Map.at( FString( Key ) ), 1 );
    }

    /**
     * Thread-safety concurrent hashing
     */

    TEST( Hash, ConcurrentHashingIsThreadSafe )
    {
        constexpr Int32 NumThreads = 8;
        constexpr Int32 NumIter    = 10000;
        const FString TestStr      = "concurrent_hash_test";
        const FHashValue Expected  = Compute( TestStr );

        TVector<std::thread> Threads;
        Threads.reserve( NumThreads );

        TAtomic<Bool> bAllMatch{ true };

        for ( Int32 T = 0; T < NumThreads; ++T )
        {
            Threads.emplace_back(
                [&] ()
                {
                    for ( Int32 I = 0; I < NumIter; ++I )
                    {
                        if ( Compute( TestStr ) != Expected )
                        {
                            bAllMatch.store( false, std::memory_order_relaxed );
                        }
                    }
                } );
        }

        for ( auto &Thread : Threads )
        {
            Thread.join();
        }

        EXPECT_TRUE( bAllMatch.load() );
    }

} // namespace Hash

} // namespace LumenEngine