/**
 * @file AssetCacheTest.cpp
 * @brief Functional tests for the TAssetCache class.
 */

#include "Assets/AssetCache.hpp"
#include "Container/Vector.hpp"

#include <gtest/gtest.h>
#include <thread>

namespace
{
struct FTestAsset
{
    LumenEngine::Int32 Value = 0;
};

using FTestAssetPtr = LumenEngine::TSharedPtr<FTestAsset>;

} // namespace

TEST( AssetCacheTest, AddAndFind )
{
    LumenEngine::Engine::TAssetCache<FTestAsset> Cache;
    FTestAssetPtr Asset = LumenEngine::MakeShared<FTestAsset>( 42 );

    Cache.Add( "TestKey", Asset );
    const FTestAssetPtr FoundAsset = Cache.Find( "TestKey" );

    ASSERT_NE( FoundAsset, nullptr );
    EXPECT_EQ( FoundAsset->Value, 42 );
    EXPECT_EQ( FoundAsset, Asset );
}

TEST( AssetCacheTest, Remove )
{
    LumenEngine::Engine::TAssetCache<FTestAsset> Cache;
    FTestAssetPtr Asset = LumenEngine::MakeShared<FTestAsset>( 42 );

    Cache.Add( "TestKey", Asset );
    EXPECT_TRUE( Cache.Remove( "TestKey" ) );
    EXPECT_EQ( Cache.Find( "TestKey" ), nullptr );
    EXPECT_FALSE( Cache.Remove( "TestKey" ) );
}

TEST( AssetCacheTest, Clear )
{
    LumenEngine::Engine::TAssetCache<FTestAsset> Cache;
    Cache.Add( "Key1", LumenEngine::MakeShared<FTestAsset>( 1 ) );
    Cache.Add( "Key2", LumenEngine::MakeShared<FTestAsset>( 2 ) );

    EXPECT_EQ( Cache.Count(), 2 );
    Cache.Clear();
    EXPECT_EQ( Cache.Count(), 0 );
}

TEST( AssetCacheTest, ThreadSafety )
{
    LumenEngine::Engine::TAssetCache<FTestAsset> Cache;
    constexpr LumenEngine::Int32 NumThreads          = 10;
    constexpr LumenEngine::Int32 IterationsPerThread = 100;

    LumenEngine::TVector<std::thread> Threads;

    for ( LumenEngine::Int32 IndexI = 0; IndexI < NumThreads; ++IndexI )
    {
        Threads.emplace_back(
            [&Cache, IndexI] ()
            {
                for ( LumenEngine::Int32 IndexJ = 0; IndexJ < IterationsPerThread; ++IndexJ )
                {
                    LumenEngine::FString Key = "Key_" + std::to_string( IndexI ) + "_" + std::to_string( IndexJ );
                    Cache.Add( Key, LumenEngine::MakeShared<FTestAsset>( ( IndexI * IterationsPerThread ) + IndexJ ) );
                    FTestAssetPtr Found = Cache.Find( Key );
                    EXPECT_NE( Found, nullptr );
                }
            } );
    }

    for ( std::thread &Thread : Threads )
    {
        Thread.join();
    }

    EXPECT_EQ( Cache.Count(), static_cast<LumenEngine::USize>( NumThreads * IterationsPerThread ) );
}

TEST( AssetCacheTest, ReferenceCounting )
{
    LumenEngine::Engine::TAssetCache<FTestAsset> Cache;
    {
        FTestAssetPtr Asset = LumenEngine::MakeShared<FTestAsset>( 100 );
        Cache.Add( "PersistentKey", Asset );
    }

    FTestAssetPtr FoundAsset = Cache.Find( "PersistentKey" );
    ASSERT_NE( FoundAsset, nullptr );
    EXPECT_EQ( FoundAsset->Value, 100 );
}
