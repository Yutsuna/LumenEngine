/**
 * @file ShaderCompilerTypes.cpp
 * @brief Implementation of shader compiler types.
 */

#include "ShaderCompiler/ShaderCompilerTypes.hpp"
#include "Bytes/Endian.hpp"

#include <algorithm>

namespace LumenEngine::Compiler
{

const FLogCategory LogShaderCompiler( "ShaderCompiler" );

/**
 * FShaderMacro
 */

Bool FShaderMacro::operator==( const FShaderMacro &Other ) const noexcept
{
    return Name == Other.Name and Value == Other.Value;
}

/**
 * FCompiledShader
 */

UInt64 FCompiledShader::GetByteSize () const noexcept
{
    return SpirV.size() * sizeof( FSpirvWord );
}

Bool FCompiledShader::IsValid () const noexcept
{
    return not SpirV.empty();
}

/**
 * FShaderCompileResult
 */

Bool FShaderCompileResult::IsSuccess () const noexcept
{
    return Shader.has_value() and Shader->IsValid() and Error == EShaderCompilerError::None;
}

FShaderCompileResult FShaderCompileResult::Success ( FCompiledShader &&InShader ) noexcept
{
    return { .Shader = std::move( InShader ), .Error = EShaderCompilerError::None, .ErrorLog = {} };
}

FShaderCompileResult FShaderCompileResult::Failure ( const EShaderCompilerError::Type InError, FString InLog ) noexcept
{
    return { .Shader = std::nullopt, .Error = InError, .ErrorLog = std::move( InLog ) };
}

/**
 * FShaderCacheMetaData
 */

TVector<Byte> FShaderCacheMetaData::Serialize () const
{
    const UInt8 EntryPointLen = static_cast<UInt8>( std::min<UInt64>( EntryPoint.size(), 255 ) );
    const USize TotalSize     = HeaderSize + EntryPointLen;

    TVector<Byte> Buffer;
    Buffer.resize( TotalSize );
    Byte *Ptr = Buffer.data();

    const auto Pack = [&Ptr] ( auto Value ) -> void
    {
        const USize Size = sizeof( Value );

        std::memcpy( Ptr, &Value, Size );
        if constexpr ( Bytes::Endian::native == Bytes::Endian::big )
        {
            std::reverse( Ptr, Ptr + Size );
        }
        Ptr += Size;
    };

    Pack( MagicNumber );                        //<< 4
    Pack( Version );                            //<< 4
    Pack( SourceHash );                         //<< 8
    Pack( static_cast<UInt8>( Stage ) );        //<< 1
    Pack( static_cast<UInt8>( Optimization ) ); //<< 1
    Pack( CompiledAtNs );                       //<< 8
    Pack( SpirVWordCount );                     //<< 4
    Pack( EntryPointLen );                      //<< 1

    if ( EntryPointLen > 0 )
    {
        std::memcpy( Ptr, EntryPoint.data(), EntryPointLen );
    }

    return Buffer;
}

TOptional<FShaderCacheMetaData> FShaderCacheMetaData::Deserialize ( std::span<const Byte> InBytes )
{
    if ( InBytes.size() < HeaderSize )
    {
        return std::nullopt;
    }

    const Byte *Cursor = InBytes.data();

    const UInt32 ReadMagic = Bytes::ReadLittleEndian<UInt32>( Cursor );
    Cursor += 4;

    const UInt32 ReadVersion = Bytes::ReadLittleEndian<UInt32>( Cursor );
    Cursor += 4;

    if ( ReadMagic != MagicNumber or ReadVersion != Version )
    {
        return std::nullopt;
    }

    FShaderCacheMetaData Meta;

    Meta.SourceHash = Bytes::ReadLittleEndian<FSourceHash>( Cursor );
    Cursor += sizeof( FSourceHash );

    Meta.Stage = static_cast<EShaderStage::Type>( Bytes::ReadLittleEndian<UInt8>( Cursor ) );
    Cursor += sizeof( UInt8 );
    Meta.Optimization = static_cast<EShaderOptimizationLevel>( Bytes::ReadLittleEndian<UInt8>( Cursor ) );
    Cursor += sizeof( UInt8 );
    Meta.CompiledAtNs = Bytes::ReadLittleEndian<UInt64>( Cursor );
    Cursor += sizeof( UInt64 );
    Meta.SpirVWordCount = Bytes::ReadLittleEndian<UInt32>( Cursor );
    Cursor += sizeof( UInt32 );

    const UInt8 EntryPointLen = Bytes::ReadLittleEndian<UInt8>( Cursor );
    Cursor += sizeof( UInt8 );

    const UInt64 BytesRead = static_cast<UInt64>( Cursor - InBytes.data() );

    if ( BytesRead + EntryPointLen > InBytes.size() )
    {
        return std::nullopt;
    }

    Meta.EntryPoint = FStringView( reinterpret_cast<const AnsiChar *>( Cursor ), EntryPointLen );

    if ( Meta.Stage >= EShaderStage::Count )
    {
        return std::nullopt;
    }

    return Meta;
}

} // namespace LumenEngine::Compiler
