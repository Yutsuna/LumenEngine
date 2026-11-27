/**
 * @file ReplicationFrame.hpp
 * @brief Fixed-layout replication wire frame and its allocation-free builder.
 */

#pragma once

#include <SceneExport.hpp>

#include <Replication/ReplicationTypes.hpp>

#include <Core/CoreTypes.hpp>

#include <Memory/Span.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

/** @brief Wire size of one frame header, excluding the trailing checksum. */
inline constexpr SCENE_EXPORT USize ReplicationFrameHeaderBytes = 56ULL;

/** @brief Wire size of one state/control record. */
inline constexpr SCENE_EXPORT USize ReplicationFrameRecordBytes = 80ULL;

/** @brief Trailing FNV-1a checksum size. */
inline constexpr SCENE_EXPORT USize ReplicationFrameChecksumBytes = 8ULL;

/** @brief Frame flag: every absolute field is present. */
inline constexpr SCENE_EXPORT UInt16 ReplicationFrameFullSnapshot = 1U;

/** @brief Frame flag: idempotent control records only, no sequence and no ack. */
inline constexpr SCENE_EXPORT UInt16 ReplicationFrameControlOnly = 2U;

/**
 * @struct FReplicationFrameHeader
 * @brief Wire header; all sequences are scoped by Direction and Channel.
 */
struct SCENE_EXPORT FReplicationFrameHeader final
{
    UInt64 Sequence                 = 0ULL;
    UInt64 AckedSequence            = 0ULL;
    UInt64 BaselineSequence         = 0ULL;
    UInt64 FrameId                  = 0ULL;
    EReplicationDirection Direction = EReplicationDirection::StateToReplica;
    EReplicationChannel Channel     = EReplicationChannel::Reliable;
    UInt16 RecordCount              = 0U;
    UInt16 Flags                    = 0U;
    UInt32 ByteLength               = 0U;
};

static_assert( Concepts::CTriviallyCopyable<FReplicationFrameHeader> );

/**
 * @struct FReplicationRecord
 * @brief Fixed-size wire record carrying absolute values, never deltas or pointers.
 */
struct SCENE_EXPORT FReplicationRecord final
{
    FPersistentActorId Actor   = InvalidPersistentActorId;
    EActorReplicationKind Kind = EActorReplicationKind::Spawn;
    UInt8 PropertyRank         = 0U;
    UInt16 PayloadSize         = 0U;
    Byte Payload[64]           = {};

    /** @brief Writes an absolute column-major local transform. */
    void SetLocalTransform ( const Maths::FMat4x4f &InTransform ) noexcept;

    /** @return The absolute local transform carried by this record. */
    [[nodiscard]] Maths::FMat4x4f GetLocalTransform () const noexcept;

    /** @brief Writes a parent persistent ID. */
    void SetParent ( const FPersistentActorId InParent ) noexcept;

    /** @return The parent persistent ID. */
    [[nodiscard]] FPersistentActorId GetParent () const noexcept;

    /** @brief Writes a visibility bit. */
    void SetVisibility ( const Bool bInVisible ) noexcept;

    /** @return The visibility bit. */
    [[nodiscard]] Bool GetVisibility () const noexcept;

    /** @brief Writes an idempotent control payload. */
    void SetControl ( const FReplicationControlPayload &InControl ) noexcept;

    /** @return The control payload. */
    [[nodiscard]] FReplicationControlPayload GetControl () const noexcept;

    /** @brief Writes a bounded input payload. */
    void SetInput ( const UInt64 InInputSequence, const Byte *InData, const UInt16 InSize ) noexcept;

    /** @return The input sequence. */
    [[nodiscard]] UInt64 GetInputSequence () const noexcept;

    /** @return The input payload span, valid while this record lives. */
    [[nodiscard]] TSpan<const Byte> GetInputPayload () const noexcept;
};

static_assert( sizeof( FReplicationRecord ) == ReplicationFrameRecordBytes );
static_assert( Concepts::CTriviallyCopyable<FReplicationRecord> );

/**
 * @struct FReplicationFrame
 * @brief Non-owning parsed frame; records stay valid until the owning builder is reset.
 */
struct SCENE_EXPORT FReplicationFrame final
{
    FReplicationFrameHeader Header;
    TSpan<const FReplicationRecord> Records;
};

/**
 * @class FReplicationFrameBuilder
 * @brief Pre-reserved serialization and parsing scratch for one replication endpoint.
 */
class SCENE_EXPORT FReplicationFrameBuilder final
{
public:

    FReplicationFrameBuilder () noexcept = default;

    /** @brief Reserves byte and record storage once; a control operation. */
    void Reserve ( USize InMaxPayloadBytes, USize InMaxRecordsPerFrame ) noexcept;

    /** @brief Clears both scratch buffers without releasing storage. */
    void Reset () noexcept;

    /** @brief Appends one record, rejecting the whole frame on overflow. */
    [[nodiscard]] TExpected<void, EReplicationError> TryAppend ( const FReplicationRecord &InRecord ) noexcept;

    /** @return The records appended to the current frame. */
    [[nodiscard]] TSpan<const FReplicationRecord> GetRecords () const noexcept;

    /** @brief Serializes the current frame into little-endian bytes with a trailing FNV-1a checksum. */
    [[nodiscard]] TExpected<USize, EReplicationError> TryWrite ( TSpan<Byte> OutBytes,
                                                                 const FReplicationFrameHeader &InHeader ) const noexcept;

    /** @brief Parses and validates one frame into the reserved record storage. */
    [[nodiscard]] TExpected<FReplicationFrame, EReplicationError> TryParse ( TSpan<const Byte> InBytes ) noexcept;

    /** @return Reserved byte capacity. */
    [[nodiscard]] USize GetByteCapacity () const noexcept;

    /** @return Reserved record capacity. */
    [[nodiscard]] USize GetRecordCapacity () const noexcept;

private:

    TVector<Byte> Buffer;
    TVector<FReplicationRecord> Records;
};

} // namespace Lumen

#include "Inline/ReplicationFrame.inl"
