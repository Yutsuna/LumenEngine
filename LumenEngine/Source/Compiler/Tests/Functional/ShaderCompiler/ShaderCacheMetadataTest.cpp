/**
 * @file ShaderCacheMetadataTest.cpp
 * @brief Tests for FShaderCacheMetaData.
 */

#include "ShaderCompiler/ShaderCompilerTypes.hpp"
#include <gtest/gtest.h>

TEST( FShaderCacheMetaData, RoundTrip )
{
    LumenEngine::Compiler::FShaderCacheMetaData Original;
    Original.SourceHash     = 0xDEADBEEFCAFEBABEULL;
    Original.Stage          = LumenEngine::Compiler::EShaderStage::Fragment;
    Original.Optimization   = LumenEngine::Compiler::EShaderOptimizationLevel::Size;
    Original.CompiledAtNs   = 1234567890ULL;
    Original.SpirVWordCount = 1024U;
    Original.EntryPoint     = "main";

    const LumenEngine::TVector<LumenEngine::Byte> Serialized = Original.Serialize();
    ASSERT_FALSE( Serialized.empty() );

    const LumenEngine::TOptional<LumenEngine::Compiler::FShaderCacheMetaData> Restored = LumenEngine::Compiler::FShaderCacheMetaData::Deserialize( Serialized );

    ASSERT_TRUE( Restored.has_value() );
    EXPECT_EQ( Restored->SourceHash, Original.SourceHash );
    EXPECT_EQ( Restored->Stage, Original.Stage );
    EXPECT_EQ( Restored->Optimization, Original.Optimization );
    EXPECT_EQ( Restored->CompiledAtNs, Original.CompiledAtNs );
    EXPECT_EQ( Restored->SpirVWordCount, Original.SpirVWordCount );
    EXPECT_EQ( Restored->EntryPoint, Original.EntryPoint );
}

TEST( FShaderCacheMetaData, TruncatedBufferReturnsNullopt )
{
    LumenEngine::Compiler::FShaderCacheMetaData Meta;
    Meta.SourceHash = 0x1ULL;
    Meta.EntryPoint = "X";

    const LumenEngine::TVector<LumenEngine::Byte> Serialized = Meta.Serialize();
    const LumenEngine::TOptional<LumenEngine::Compiler::FShaderCacheMetaData> Restored =
        LumenEngine::Compiler::FShaderCacheMetaData::Deserialize( std::span<const LumenEngine::Byte>( Serialized.data(), Serialized.size() - 1 ) );

    EXPECT_FALSE( Restored.has_value() );
}

TEST( FShaderCacheMetaData, WrongMagicReturnsNullopt )
{
    LumenEngine::Compiler::FShaderCacheMetaData Meta;
    Meta.SourceHash = 0x2ULL;

    LumenEngine::TVector<LumenEngine::Byte> Serialized = Meta.Serialize();
    Serialized[0]                                      = 0xFF;
    Serialized[1]                                      = 0xFF;

    const LumenEngine::TOptional<LumenEngine::Compiler::FShaderCacheMetaData> Restored = LumenEngine::Compiler::FShaderCacheMetaData::Deserialize( Serialized );
    EXPECT_FALSE( Restored.has_value() );
}

TEST( FShaderCacheMetaData, WrongVersionReturnsNullopt )
{
    LumenEngine::Compiler::FShaderCacheMetaData Meta;
    Meta.SourceHash = 0x3ULL;

    LumenEngine::TVector<LumenEngine::Byte> Serialized = Meta.Serialize();
    Serialized[4]                                      = 0xFF;
    Serialized[5]                                      = 0xFF;

    const LumenEngine::TOptional<LumenEngine::Compiler::FShaderCacheMetaData> Restored = LumenEngine::Compiler::FShaderCacheMetaData::Deserialize( Serialized );
    EXPECT_FALSE( Restored.has_value() );
}
