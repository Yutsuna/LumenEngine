/**
 * @file LumenCompilerTypes.cpp
 * @brief Implementation of LumenCompilerTypes
 */

#include "LumenCompiler/LumenCompilerTypes.hpp"
#include "Bytes/Endian.hpp"

const LumenEngine::FLogCategory LumenEngine::Compiler::LogLumenCompiler( "LumenCompiler" );

LumenEngine::TVector<LumenEngine::Byte> LumenEngine::Compiler::FLumenCacheMetaData::Serialize () const
{
    const UInt8 BlockNameLen = static_cast<UInt8>( std::min<USize>( BlockName.size(), 255U ) );
    const USize TotalSize    = static_cast<USize>( HeaderSize + BlockNameLen );

    TVector<Byte> Buffer;
    Buffer.resize( TotalSize );
    Byte *Ptr = Buffer.data();

    const auto Pack = [&Ptr] ( auto InValue ) -> void
    {
        const USize Size = sizeof( InValue );
        std::memcpy( Ptr, &InValue, Size );

        if constexpr ( Bytes::Endian::native == Bytes::Endian::big )
        {
            std::reverse( Ptr, Ptr + Size );
        }
        Ptr += Size;
    };

    Pack( MagicNumber );                      //<< [4] magic
    Pack( Version );                          //<< [4] version
    Pack( SourceHash );                       //<< [8] hash
    Pack( static_cast<UInt32>( AssetType ) ); //<< [4] asset type
    Pack( CompiledAtNs );                     //<< [8] timestamp
    Pack( BlobByteCount );                    //<< [4] blob size
    Pack( BlockNameLen );                     //<< [1] block name length

    if ( BlockNameLen > 0 )
    {
        std::memcpy( Ptr, BlockName.data(), BlockNameLen );
    }

    return Buffer;
}

LumenEngine::TOptional<LumenEngine::Compiler::FLumenCacheMetaData> LumenEngine::Compiler::FLumenCacheMetaData::Deserialize ( std::span<const Byte> InBytes )
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

    FLumenCacheMetaData Meta;

    Meta.SourceHash = Bytes::ReadLittleEndian<FAssetHash>( Cursor );
    Cursor += sizeof( FAssetHash );

    Meta.AssetType = static_cast<EAssetType::Type>( Bytes::ReadLittleEndian<UInt32>( Cursor ) );
    Cursor += sizeof( UInt32 );

    Meta.CompiledAtNs = Bytes::ReadLittleEndian<UInt64>( Cursor );
    Cursor += sizeof( UInt64 );

    Meta.BlobByteCount = Bytes::ReadLittleEndian<UInt32>( Cursor );
    Cursor += sizeof( UInt32 );

    const UInt8 BlockNameLen = Bytes::ReadLittleEndian<UInt8>( Cursor );
    Cursor += sizeof( UInt8 );

    const USize BytesConsumed = static_cast<USize>( Cursor - InBytes.data() );
    if ( BytesConsumed + BlockNameLen > InBytes.size() )
    {
        return std::nullopt;
    }

    if ( BlockNameLen > 0 )
    {
        Meta.BlockName = FString( reinterpret_cast<const AnsiChar *>( Cursor ), BlockNameLen );
    }

    if ( Meta.AssetType != EAssetType::Mesh and Meta.AssetType != EAssetType::Material and Meta.AssetType != EAssetType::Unknown )
    {
        return std::nullopt;
    }

    return Meta;
}
