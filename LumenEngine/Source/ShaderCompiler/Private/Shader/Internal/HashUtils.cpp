/**
 * @file HashUtils.cpp
 * @brief Implementation of high-performance hashing utilities.
 */

#include "Shader/Internal/HashUtils.hpp"

namespace
{

constexpr LumenEngine::FSourceHash FNV1aOffsetBasis = 0XCBF29CE484222325ULL;
constexpr LumenEngine::FSourceHash FNV1aPrime       = 0X100000001B3ULL;

} // namespace

void LumenEngine::Internal::FHashUtils::CombineHash ( FSourceHash &InOutHash, const void *InData, const USize InSize ) noexcept
{
    const Byte *DataBytes = static_cast<const Byte *>( InData );
    for ( USize Index = 0ULL; Index < InSize; ++Index )
    {
        InOutHash ^= DataBytes[Index];
        InOutHash *= FNV1aPrime;
    }
}

LumenEngine::FSourceHash LumenEngine::Internal::FHashUtils::ComputeRequestHash ( FStringView InSource, const FShaderCompileRequest &InRequest ) noexcept
{
    FSourceHash Hash = FNV1aOffsetBasis;

    CombineHash( Hash, InSource.data(), InSource.size() );
    CombineHash( Hash, &InRequest.Stage, sizeof( InRequest.Stage ) );
    CombineHash( Hash, &InRequest.Profile, sizeof( InRequest.Profile ) );
    CombineHash( Hash, &InRequest.Optimization, sizeof( InRequest.Optimization ) );
    CombineHash( Hash, InRequest.EntryPoint.c_str(), InRequest.EntryPoint.size() );
    CombineHash( Hash, &InRequest.GlslVersion, sizeof( InRequest.GlslVersion ) );
    CombineHash( Hash, &InRequest.bEmitDebugInfo, sizeof( InRequest.bEmitDebugInfo ) );

    for ( const FShaderMacro &Macro : InRequest.Macros )
    {
        CombineHash( Hash, Macro.Name.data(), Macro.Name.size() );
        CombineHash( Hash, Macro.Value.data(), Macro.Value.size() );
    }

    return Hash;
}