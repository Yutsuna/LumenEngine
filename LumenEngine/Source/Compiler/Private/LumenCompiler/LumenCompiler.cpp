/**
 * @file LumenCompiler.cpp
 * @brief Implementation of LumenCompiler orchestrator and hashing hooks.
 */

#include "LumenCompiler/LumenCompiler.hpp"
#include "LumenCompiler/Internal/BinarySerializer.hpp"
#include "LumenCompiler/Internal/DLSLParser.hpp"

#include "Container/File.hpp"
#include "HAL/Memory/LinearAllocator.hpp"

/**
 * Ctor
 */

LumenEngine::Compiler::FLumenCompiler::FLumenCompiler ( FLumenCompilerConfig InConfig ) noexcept : TCompiler( std::move( InConfig ) )
{
    ScratchBuffer.reserve( Config.ScratchBufferSize );
}

/**
 * Compile Frontends
 */

LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompiler::CompileAsset ( const FLumenCompileRequest &InRequest ) noexcept
{
    return TCompiler::Compile( InRequest );
}

LumenEngine::Compiler::FLumenCompileResult LumenEngine::Compiler::FLumenCompiler::CompileAssetFromSource ( FStringView InSource,
                                                                                                           const FLumenCompileRequest &InRequest ) noexcept
{
    return TCompiler::CompileFromSource( InSource, InRequest );
}

/**
 * Cache Management
 */

LumenEngine::USize LumenEngine::Compiler::FLumenCompiler::WarmCache () noexcept
{
    USize LoadedCount = 0ULL;
    std::error_code Ec;

    if ( not std::filesystem::exists( Config.CacheDirectory, Ec ) )
    {
        return LoadedCount;
    }

    for ( const std::filesystem::directory_entry &Entry : std::filesystem::directory_iterator( Config.CacheDirectory, Ec ) )
    {
        if ( Entry.path().extension() == ".meta" )
        {
            if ( const TOptional<TVector<Byte>> MetaBytes = FIOFile::ReadAllBytes<Byte>( Entry.path().string() ) )
            {
                if ( const TOptional<Compiler::FLumenCacheMetaData> MetaOpt = FLumenCacheMetaData::Deserialize( std::span<const Byte>( *MetaBytes ) ) )
                {
                    FLumenCompileRequest Request;
                    Request.TargetBlockName = MetaOpt->BlockName;

                    if ( Cache->TryGet( MetaOpt->SourceHash, Request ) )
                    {
                        ++LoadedCount;
                    }
                }
            }
        }
    }
    return LoadedCount;
}

LumenEngine::USize LumenEngine::Compiler::FLumenCompiler::ClearCache () noexcept
{
    return Cache->Clear();
}

LumenEngine::USize LumenEngine::Compiler::FLumenCompiler::ClearStaleCache ( Float64 InMaxAgeSeconds ) noexcept
{
    return Cache->ClearStale( InMaxAgeSeconds );
}

/**
 * Hooks for TCompiler
 */

LumenEngine::Compiler::FAssetHash LumenEngine::Compiler::FLumenCompiler::ComputeHash ( FStringView InSource, const FLumenCompileRequest &InRequest ) noexcept
{
    FAssetHash Hash        = 0XCBF29CE484222325ULL;
    constexpr UInt64 Prime = 0X100000001B3ULL;

    const auto Combine = [&Hash] ( const void *InData, USize InSize )
    {
        const Byte *Ptr = static_cast<const Byte *>( InData );
        for ( USize Index = 0; Index < InSize; ++Index )
        {
            Hash ^= Ptr[Index];
            Hash *= Prime;
        }
    };

    Combine( InSource.data(), InSource.size() );
    Combine( InRequest.TargetBlockName.data(), InRequest.TargetBlockName.size() );
    Combine( InRequest.ExpectedBlockType.data(), InRequest.ExpectedBlockType.size() );

    return Hash;
}

LumenEngine::Bool LumenEngine::Compiler::FLumenCompiler::TryReflect ( FCompiledLumenAsset & /*InOutCompiled*/, FString & /*OutErrorLog*/ ) noexcept
{
    return true;
}

LumenEngine::Bool LumenEngine::Compiler::FLumenCompiler::CompileFresh (
    FStringView InSource, const FLumenCompileRequest &InRequest, FAssetHash InHash, FCompiledLumenAsset &OutCompiled, FString &OutErrorLog )
{
    ScratchBuffer.resize( Config.ScratchBufferSize );
    HAL::FLinearAllocator Allocator( ScratchBuffer.data(), Config.ScratchBufferSize );

    FDLSLParser Parser( InSource, Allocator );
    const auto DocResult = Parser.Parse();

    if ( not DocResult.has_value() )
    {
        OutErrorLog = DocResult.error();
        return false;
    }

    const FDLSLRootBlock *TargetBlock = FindTargetBlock( DocResult.value(), InRequest );
    if ( TargetBlock == nullptr )
    {
        OutErrorLog = "Target block not found.";
        return false;
    }

    if ( not InRequest.ExpectedBlockType.empty() and TargetBlock->BlockType != InRequest.ExpectedBlockType )
    {
        OutErrorLog = "Block type mismatch.";
        return false;
    }

    const auto SerializedResult = SerializeBlock( TargetBlock );
    if ( not SerializedResult.has_value() )
    {
        OutErrorLog = SerializedResult.error();
        return false;
    }

    OutCompiled.BinaryBlob = SerializedResult.value();
    OutCompiled.AssetType  = ResolveAssetType( TargetBlock->BlockType );
    OutCompiled.Hash       = InHash;
    OutCompiled.BlockName  = TargetBlock->Name;
    OutCompiled.bFromCache = false;

    return true;
}

/**
 * Internal Block Resolution
 */

const LumenEngine::Compiler::FDLSLRootBlock *LumenEngine::Compiler::FLumenCompiler::FindTargetBlock ( const FDLSLDocument *InDocument,
                                                                                                      const FLumenCompileRequest &InRequest ) noexcept
{
    if ( InDocument == nullptr )
    {
        return nullptr;
    }

    for ( const FDLSLRootBlock *Block = InDocument->FirstBlock; Block != nullptr; Block = Block->Next )
    {
        if ( InRequest.TargetBlockName.empty() )
        {
            return Block;
        }
        if ( Block->Name == InRequest.TargetBlockName )
        {
            return Block;
        }
    }

    return nullptr;
}

LumenEngine::TExpected<LumenEngine::TVector<LumenEngine::Byte>, LumenEngine::FString>
LumenEngine::Compiler::FLumenCompiler::SerializeBlock ( const FDLSLRootBlock *InBlock )
{
    if ( InBlock->BlockType == "Mesh" )
    {
        return FBinarySerializer::SerializeMesh( InBlock );
    }

    if ( InBlock->BlockType == "Material" )
    {
        return FBinarySerializer::SerializeMaterial( InBlock );
    }

    return MakeUnexpected( "Unknown block type." );
}

LumenEngine::Compiler::EAssetType::Type LumenEngine::Compiler::FLumenCompiler::ResolveAssetType ( FStringView InBlockType ) noexcept
{
    if ( InBlockType == "Mesh" )
    {
        return EAssetType::Mesh;
    }

    if ( InBlockType == "Material" )
    {
        return EAssetType::Material;
    }

    return EAssetType::Unknown;
}
