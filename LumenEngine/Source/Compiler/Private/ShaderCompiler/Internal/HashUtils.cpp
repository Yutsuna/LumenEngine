/**
 * @file HashUtils.cpp
 * @brief Implementation of high-performance hashing utilities.
 */

#include "ShaderCompiler/Internal/HashUtils.hpp"

namespace LumenEngine::Compiler::Internal
{

namespace
{

    constexpr FSourceHash FNV1aOffsetBasis = 0XCBF29CE484222325ULL;
    constexpr FSourceHash FNV1aPrime       = 0X100000001B3ULL;

} // namespace

void FHashUtils::CombineHash ( FSourceHash &InOutHash, const void *InData, const USize InSize ) noexcept
{
    const Byte *DataBytes = static_cast<const Byte *>( InData );
    for ( USize Index = 0ULL; Index < InSize; ++Index )
    {
        InOutHash ^= DataBytes[Index];
        InOutHash *= FNV1aPrime;
    }
}

FSourceHash FHashUtils::ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    FSourceHash Hash = FNV1aOffsetBasis;

    FHashUtils::CombineHash( Hash, InSource.data(), InSource.size() );
    FHashUtils::CombineHash( Hash, &InRequest.Stage, sizeof( InRequest.Stage ) );
    FHashUtils::CombineHash( Hash, &InRequest.Profile, sizeof( InRequest.Profile ) );
    FHashUtils::CombineHash( Hash, &InRequest.Optimization, sizeof( InRequest.Optimization ) );
    FHashUtils::CombineHash( Hash, InRequest.EntryPoint.c_str(), InRequest.EntryPoint.size() );
    FHashUtils::CombineHash( Hash, &InRequest.GlslVersion, sizeof( InRequest.GlslVersion ) );
    FHashUtils::CombineHash( Hash, &InRequest.bEmitDebugInfo, sizeof( InRequest.bEmitDebugInfo ) );

    for ( const FShaderMacro &Macro : InRequest.Macros )
    {
        FHashUtils::CombineHash( Hash, Macro.Name.data(), Macro.Name.size() );
        FHashUtils::CombineHash( Hash, Macro.Value.data(), Macro.Value.size() );
    }

    return Hash;
}

} // namespace LumenEngine::Compiler::Internal
