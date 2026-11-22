/**
 * @file RHIShaderPackageCodec.cpp
 * @brief Implements bounded versioned shader package serialization.
 */

#include <RHI/Shaders/RHIShaderPackageCodec.hpp>
#include <RHI/Types/RHIHash.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

#include <Concepts/TriviallyCopyable.hpp>

#include <cstring>

/**
 * Private
 */

namespace
{

/**
 * TODO: check if there is not already a Filesystem or any function similar in Core/
 */

template <class TObject>
    requires Lumen::Concepts::CTriviallyCopyable<TObject>
void WriteValue ( Lumen::TVector<Lumen::Byte> &OutBytes, const TObject &InValue ) noexcept
{
    const auto *Begin = reinterpret_cast<const Lumen::Byte *>( std::addressof( InValue ) );

    OutBytes.insert( OutBytes.end(), Begin, Begin + sizeof( TObject ) );
}

/**
 * TODO: check if there is not already a Filesystem or any function similar in Core/
 */

template <class TObject>
    requires Lumen::Concepts::CTriviallyCopyable<TObject>
Lumen::Bool ReadValue ( Lumen::TSpan<const Lumen::Byte> InBytes, Lumen::USize &InOutOffset, TObject &OutValue ) noexcept
{
    if ( InOutOffset > InBytes.size() or sizeof( TObject ) > InBytes.size() - InOutOffset )
    {
        return false;
    }
    std::memcpy( std::addressof( OutValue ), InBytes.data() + InOutOffset, sizeof( TObject ) );
    InOutOffset += sizeof( TObject );
    return true;
}

void WriteBytes ( Lumen::TVector<Lumen::Byte> &OutBytes, const Lumen::TSpan<const Lumen::Byte> InBytes ) noexcept
{
    OutBytes.insert( OutBytes.end(), InBytes.begin(), InBytes.end() );
}

Lumen::Bool ReadBytes ( Lumen::TSpan<const Lumen::Byte> InBytes,
                        Lumen::USize &InOutOffset,
                        Lumen::TVector<Lumen::Byte> &OutBytes,
                        const Lumen::UInt32 InSize ) noexcept
{
    if ( InOutOffset > InBytes.size() or InSize > InBytes.size() - InOutOffset )
    {
        return false;
    }
    OutBytes.assign( InBytes.begin() + static_cast<Lumen::ISize>( InOutOffset ),
                     InBytes.begin() + static_cast<Lumen::ISize>( InOutOffset + InSize ) );
    InOutOffset += InSize;
    return true;
}

Lumen::RHI::FRHIError CodecError () noexcept
{
    return { Lumen::RHI::ERHIErrorCode::CorruptedData, Lumen::RHI::ERHIErrorCondition::Shader,
             "Invalid or truncated shader package" };
}

} // namespace

Lumen::TExpected<Lumen::TVector<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::EncodeRHIShaderPackage ( const FRHIShaderPackage &InPackage ) noexcept
{
    Lumen::TVector<Lumen::Byte> Result;

    WriteBytes( Result, Lumen::TSpan<const Lumen::Byte>( reinterpret_cast<const Lumen::Byte *>( "LRHI" ), 4U ) );
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

    /** NOTE: The checksum is appended last so all existing payload offsets remain unchanged. */
    const UInt64 Checksum = RHIHashBytes( TSpan<const Byte>( Result.data(), Result.size() ) );
    WriteValue( Result, Checksum );
    return Result;
}

Lumen::TExpected<Lumen::RHI::FRHIShaderPackage, Lumen::RHI::FRHIError>
Lumen::RHI::DecodeRHIShaderPackage ( const TSpan<const Byte> InBytes ) noexcept
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
        LUMEN_EXPECT( _, Package.AddEntry( std::move( Entry ) ) );
    }
    if ( InBytes.size() - Offset != sizeof( UInt64 ) )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    const USize PayloadSize = Offset;
    UInt64 StoredChecksum   = 0U;
    if ( not ReadValue( InBytes, Offset, StoredChecksum ) )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    const UInt64 ComputedChecksum = RHIHashBytes( TSpan<const Byte>( InBytes.data(), PayloadSize ) );
    if ( ComputedChecksum != StoredChecksum )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    if ( Offset != InBytes.size() )
    {
        return TUnexpected<FRHIError>( CodecError() );
    }
    return Package;
}
