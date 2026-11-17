/**
 * @file RHIShaderPackageCodec.cpp
 * @brief Implements bounded versioned shader package serialization.
 */
#include "Concepts/TriviallyCopyable.hpp"
#include <Shaders/RHIShaderPackageCodec.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <cstring>
#include <type_traits>

namespace Lumen
{
namespace
{

    template <typename ValueType>
        requires Concepts::CTriviallyCopyable<ValueType>
    void WriteValue ( TVector<Byte> &OutBytes, const ValueType &InValue ) noexcept
    {
        const auto *Begin = reinterpret_cast<const Byte *>( std::addressof( InValue ) );
        OutBytes.insert( OutBytes.end(), Begin, Begin + sizeof( ValueType ) );
    }

    template <typename ValueType>
        requires Concepts::CTriviallyCopyable<ValueType>
    Bool ReadValue ( TSpan<const Byte> InBytes, USize &InOutOffset, ValueType &OutValue ) noexcept
    {
        if ( InOutOffset > InBytes.size() or sizeof( ValueType ) > InBytes.size() - InOutOffset )
        {
            return false;
        }
        std::memcpy( std::addressof( OutValue ), InBytes.data() + InOutOffset, sizeof( ValueType ) );
        InOutOffset += sizeof( ValueType );
        return true;
    }

    void WriteBytes ( TVector<Byte> &OutBytes, const TSpan<const Byte> InBytes ) noexcept
    {
        OutBytes.insert( OutBytes.end(), InBytes.begin(), InBytes.end() );
    }

    Bool ReadBytes ( TSpan<const Byte> InBytes, USize &InOutOffset, TVector<Byte> &OutBytes, const UInt32 InSize ) noexcept
    {
        if ( InOutOffset > InBytes.size() or InSize > InBytes.size() - InOutOffset )
        {
            return false;
        }
        OutBytes.assign( InBytes.begin() + static_cast<ISize>( InOutOffset ),
                         InBytes.begin() + static_cast<ISize>( InOutOffset + InSize ) );
        InOutOffset += InSize;
        return true;
    }

    FRHIError CodecError () noexcept
    {
        return { ERHIErrorCode::CorruptedData, ERHIErrorCondition::Shader, "Invalid or truncated shader package" };
    }

} // namespace
} // namespace Lumen

Lumen::TExpected<Lumen::TVector<Lumen::Byte>, Lumen::FRHIError>
Lumen::EncodeRHIShaderPackage ( const FRHIShaderPackage &InPackage ) noexcept
{
    TVector<Byte> Result;
    WriteBytes( Result, TSpan<const Byte>( reinterpret_cast<const Byte *>( "LRHI" ), 4U ) );
    WriteValue( Result, 1U );
    const TOptional<UInt32> Count = RHICheckedCast<UInt32>( InPackage.GetEntries().size() );
    if ( not Count )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    WriteValue( Result, *Count );
    for ( const FRHIShaderPackageEntry &Entry : InPackage.GetEntries() )
    {
        const TOptional<UInt32> ByteCount    = RHICheckedCast<UInt32>( Entry.Bytecode.size() );
        const TOptional<UInt32> EntryCount   = RHICheckedCast<UInt32>( Entry.EntryPoint.size() );
        const TOptional<UInt32> BindingCount = RHICheckedCast<UInt32>( Entry.Reflection.Bindings.size() );
        if ( not ByteCount or not EntryCount or not BindingCount )
        {
            return TUnexpected<FRHIError>( CodecError() );
        }
        WriteValue( Result, Entry.Stage );
        WriteValue( Result, Entry.Format );
        WriteValue( Result, *ByteCount );
        WriteBytes( Result, TSpan<const Byte>( Entry.Bytecode.data(), Entry.Bytecode.size() ) );
        WriteValue( Result, *EntryCount );
        WriteBytes( Result,
                    TSpan<const Byte>( reinterpret_cast<const Byte *>( Entry.EntryPoint.data() ), Entry.EntryPoint.size() ) );
        WriteValue( Result, Entry.Reflection.WorkgroupSize );
        WriteValue( Result, Entry.Reflection.InlineConstantBytes );
        WriteValue( Result, *BindingCount );
        for ( const auto &Binding : Entry.Reflection.Bindings )
        {
            WriteValue( Result, Binding );
        }
    }
    return Result;
}

Lumen::TExpected<Lumen::FRHIShaderPackage, Lumen::FRHIError>
Lumen::DecodeRHIShaderPackage ( const TSpan<const Byte> InBytes ) noexcept
{
    USize Offset   = 0U;
    UInt32 Magic   = 0U;
    UInt32 Version = 0U;
    UInt32 Count   = 0U;
    if ( not ReadValue( InBytes, Offset, Magic ) or Magic != 0x4948524CU or not ReadValue( InBytes, Offset, Version ) or
         Version != 1U or not ReadValue( InBytes, Offset, Count ) )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    FRHIShaderPackage Package;
    for ( UInt32 EntryIndex = 0U; EntryIndex < Count; ++EntryIndex )
    {
        FRHIShaderPackageEntry Entry;
        UInt32 Size = 0U;
        if ( not ReadValue( InBytes, Offset, Entry.Stage ) or not ReadValue( InBytes, Offset, Entry.Format ) or
             not ReadValue( InBytes, Offset, Size ) or not ReadBytes( InBytes, Offset, Entry.Bytecode, Size ) or
             not ReadValue( InBytes, Offset, Size ) )
        {
            return TUnexpected<FRHIError>( CodecError() );
        }
        TVector<Byte> Text;
        if ( not ReadBytes( InBytes, Offset, Text, Size ) )
        {
            return TUnexpected<FRHIError>( CodecError() );
        }
        Entry.EntryPoint.assign( reinterpret_cast<const AnsiChar *>( Text.data() ), Text.size() );
        UInt32 BindingCount = 0U;
        if ( not ReadValue( InBytes, Offset, Entry.Reflection.WorkgroupSize ) or
             not ReadValue( InBytes, Offset, Entry.Reflection.InlineConstantBytes ) or
             not ReadValue( InBytes, Offset, BindingCount ) )
        {
            return TUnexpected<FRHIError>( CodecError() );
        }
        for ( UInt32 BindingIndex = 0U; BindingIndex < BindingCount; ++BindingIndex )
        {
            FRHIShaderBindingReflection Binding;
            if ( not ReadValue( InBytes, Offset, Binding ) )
            {
                return TUnexpected<FRHIError>( CodecError() );
            }
            Entry.Reflection.Bindings.push_back( Binding );
        }
        if ( const TExpected<void, FRHIError> Added = Package.AddEntry( std::move( Entry ) ); not Added )
        {
            return TUnexpected<FRHIError>( Added.Error() );
        }
    }
    if ( Offset != InBytes.size() )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    return Package;
}
