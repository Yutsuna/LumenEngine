/**
 * @file Endian.inl
 * @brief Implementation of endian conversion functions.
 */

#pragma once

#include "Bytes/Endian.hpp"

#include <algorithm>
#include <cstring>

/**
 * Write
 */

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
void LumenEngine::Bytes::Write ( TVector<Byte> &OutBuffer, const Type Value, const Endian ByteOrder ) noexcept
{
    constexpr USize TypeSize = sizeof( Type );

    Byte Bytes[TypeSize];
    std::memcpy( Bytes, &Value, TypeSize );

    if ( ByteOrder != Endian::native )
    {
        std::reverse( Bytes, Bytes + TypeSize );
    }

    OutBuffer.insert( OutBuffer.end(), Bytes, Bytes + TypeSize );
}

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
void LumenEngine::Bytes::WriteLittleEndian ( TVector<Byte> &OutBuffer, const Type Value ) noexcept
{
    Write<Type>( OutBuffer, Value, Endian::little );
}

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
void LumenEngine::Bytes::WriteBigEndian ( TVector<Byte> &OutBuffer, const Type Value ) noexcept
{
    Write<Type>( OutBuffer, Value, Endian::big );
}

/**
 * Read
 */

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
Type LumenEngine::Bytes::Read ( const TVector<Byte> &Buffer, const Endian ByteOrder ) noexcept
{
    constexpr USize TypeSize = sizeof( Type );

    Byte Bytes[TypeSize];
    std::memcpy( Bytes, Buffer.data(), TypeSize );

    if ( ByteOrder != Endian::native )
    {
        std::reverse( Bytes, Bytes + TypeSize );
    }

    Type Result;
    std::memcpy( &Result, Bytes, TypeSize );
    return Result;
}

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
Type LumenEngine::Bytes::ReadLittleEndian ( const TVector<Byte> &Buffer ) noexcept
{
    return Read<Type>( Buffer, Endian::little );
}

template <typename Type>
    requires LumenEngine::Concepts::CTriviallyCopyable<Type>
Type LumenEngine::Bytes::ReadBigEndian ( const TVector<Byte> &Buffer ) noexcept
{
    return Read<Type>( Buffer, Endian::big );
}
