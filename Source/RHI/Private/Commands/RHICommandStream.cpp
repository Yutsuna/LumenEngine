/**
 * @file RHICommandStream.cpp
 * @brief Implements chunked aligned command storage.
 */

#include <Commands/RHICommandStream.hpp>
#include <Utilities/RHIAlignment.hpp>
#include <Utilities/RHICheckedMath.hpp>
#include <cstring>
namespace
{
constexpr Lumen::UInt32 CommandMagic = 0x52434844U;
Lumen::FRHIError StreamError ( Lumen::FStringView InMessage ) noexcept
{
    return { Lumen::ERHIErrorCode::CorruptedData, Lumen::ERHIErrorCondition::Command, InMessage };
}
} // namespace
Lumen::FRHICommandStream::FRHICommandStream ( const USize InChunkCapacity ) noexcept
    : ChunkCapacity( InChunkCapacity < 256U ? 256U : InChunkCapacity )
{
}
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHICommandStream::WritePayload ( const UInt32 InOpcode,
                                                                                  const TSpan<const Byte> InPayload ) noexcept
{
    const auto PayloadSize = RHICheckedCast<UInt32>( InPayload.size() );
    if ( not PayloadSize )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Command payload is too large" ) );
    }
    if ( Chunks.empty() )
    {
        Chunks.emplace_back();
        CurrentChunk = 0U;
    }
    const auto AlignedOffset = RHIAlignUp( Chunks[CurrentChunk].size(), alignof( FHeader ) );
    const auto Required =
        AlignedOffset ? RHICheckedAdd( *AlignedOffset - Chunks[CurrentChunk].size() + sizeof( FHeader ), InPayload.size() )
                      : TOptional<USize>{};
    if ( not Required )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Command payload is too large" ) );
    }
    if ( Chunks[CurrentChunk].size() > ChunkCapacity or *Required > ChunkCapacity - Chunks[CurrentChunk].size() )
    {
        ++CurrentChunk;
        if ( CurrentChunk == Chunks.size() )
        {
            Chunks.emplace_back();
        }
    }
    TVector<Byte> &Chunk = Chunks[CurrentChunk];
    const USize Offset   = RHIAlignUp( Chunk.size(), alignof( FHeader ) ).ValueOr( Chunk.size() );
    Chunk.resize( Offset + sizeof( FHeader ) + InPayload.size() );
    const FHeader Header{ .Magic = CommandMagic, .Opcode = InOpcode, .PayloadSize = *PayloadSize, .Reserved = 0U };
    std::memcpy( Chunk.data() + Offset, std::addressof( Header ), sizeof( Header ) );
    if ( not InPayload.empty() )
    {
        std::memcpy( Chunk.data() + Offset + sizeof( Header ), InPayload.data(), InPayload.size() );
    }
    Records.push_back( { CurrentChunk, Offset, *PayloadSize } );
    return {};
}
Lumen::TExpected<Lumen::FRHICommandView, Lumen::FRHIError> Lumen::FRHICommandStream::Read ( const USize InIndex ) const noexcept
{
    if ( InIndex >= Records.size() )
    {
        return TUnexpected<FRHIError>( StreamError( "Command index is outside the stream" ) );
    }
    const FRecord &Record = Records[InIndex];
    if ( Record.ChunkIndex >= Chunks.size() )
    {
        return TUnexpected<FRHIError>( StreamError( "Command chunk is invalid" ) );
    }
    const TVector<Byte> &Chunk = Chunks[Record.ChunkIndex];
    const auto End             = RHICheckedAdd( Record.Offset, sizeof( FHeader ) );
    const auto PayloadEnd      = End ? RHICheckedAdd( *End, static_cast<USize>( Record.PayloadSize ) ) : TOptional<USize>{};
    if ( not PayloadEnd or *PayloadEnd > Chunk.size() or Record.Offset % alignof( FHeader ) != 0U )
    {
        return TUnexpected<FRHIError>( StreamError( "Command range is truncated or misaligned" ) );
    }
    FHeader Header{};
    std::memcpy( std::addressof( Header ), Chunk.data() + Record.Offset, sizeof( Header ) );
    if ( Header.Magic != CommandMagic or Header.Reserved != 0U or Header.PayloadSize != Record.PayloadSize )
    {
        return TUnexpected<FRHIError>( StreamError( "Command header is corrupted" ) );
    }
    return FRHICommandView{ .Opcode  = Header.Opcode,
                            .Payload = TSpan<const Byte>( Chunk.data() + *End, static_cast<USize>( Header.PayloadSize ) ) };
}
void Lumen::FRHICommandStream::Retain ( TSharedPtr<IRHIObject> InObject ) noexcept
{
    if ( InObject )
    {
        RetainedObjects.push_back( std::move( InObject ) );
    }
}
void Lumen::FRHICommandStream::Clear () noexcept
{
    for ( auto &Chunk : Chunks )
    {
        Chunk.clear();
    }
    CurrentChunk = 0U;
    Records.clear();
    RetainedObjects.clear();
}
Lumen::USize Lumen::FRHICommandStream::GetCommandCount () const noexcept
{
    return Records.size();
}
Lumen::USize Lumen::FRHICommandStream::GetChunkCount () const noexcept
{
    return Chunks.size();
}
Lumen::USize Lumen::FRHICommandStream::GetRetainedCount () const noexcept
{
    return RetainedObjects.size();
}
