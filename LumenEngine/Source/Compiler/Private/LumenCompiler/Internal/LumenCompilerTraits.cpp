/**
 * @file LumenCompilerTraits.cpp
 * @brief Implementation of LumenCompilerTraits
 */

#include "LumenCompiler/Internal/LumenCompilerTraits.hpp"
#include "LumenCompiler/LumenCompilerTypes.hpp"

#include <format>

LumenEngine::FString LumenEngine::Compiler::FLumenCompilerTraits::GetSourcePath ( const FLumenCompileRequest &InRequest ) noexcept
{
    return InRequest.SourcePath;
}

LumenEngine::FString LumenEngine::Compiler::FLumenCompilerTraits::BuildCachePath ( const Filesystem::FPath &InCacheDir,
                                                                                   FAssetHash InHash,
                                                                                   const FLumenCompileRequest &InRequest,
                                                                                   const AnsiChar *InExt ) noexcept
{
    const FString Suffix = InRequest.TargetBlockName.empty() ? FString{} : ( FString( "_" ) + InRequest.TargetBlockName );

    return ( InCacheDir / std::format( "{:016x}{}{}", InHash, Suffix, InExt ) ).ToString();
}

LumenEngine::Bool
LumenEngine::Compiler::FLumenCompilerTraits::IsValidMeta ( const FLumenCacheMetaData &InMeta, FAssetHash InHash, const FLumenCompileRequest &InRequest ) noexcept
{
    if ( InMeta.SourceHash != InHash )
    {
        return false;
    }

    if ( not InRequest.TargetBlockName.empty() and InMeta.BlockName != InRequest.TargetBlockName )
    {
        return false;
    }

    return true;
}

LumenEngine::Compiler::FCompiledLumenAsset
LumenEngine::Compiler::FLumenCompilerTraits::RestoreFromCache ( FAssetHash InHash, const FLumenCacheMetaData &InMeta, TVector<Byte> &&InBinary ) noexcept
{
    return { .BinaryBlob = std::move( InBinary ), .AssetType = InMeta.AssetType, .Hash = InHash, .BlockName = InMeta.BlockName, .bFromCache = true };
}

LumenEngine::Compiler::FLumenCacheMetaData
LumenEngine::Compiler::FLumenCompilerTraits::CreateMeta ( FAssetHash InHash, const FLumenCompileRequest &InRequest, const FCompiledLumenAsset &InCompiled ) noexcept
{
    return { .SourceHash    = InHash,
             .AssetType     = InCompiled.AssetType,
             .CompiledAtNs  = 0, //<< Set by TCompilerCache when writing to disk
             .BlobByteCount = static_cast<UInt32>( InCompiled.BinaryBlob.size() ),
             .BlockName     = InRequest.TargetBlockName };
}

const LumenEngine::TVector<LumenEngine::Byte> &LumenEngine::Compiler::FLumenCompilerTraits::GetBinaryData ( const FCompiledLumenAsset &InCompiled ) noexcept
{
    return InCompiled.BinaryBlob;
}