/**
 * @file OpenGLMemoryObjects.cpp
 * @brief Implements OpenGL buffer recycling, transient placement and defrag planning.
 */

#include <RHIOpenGL/Memory/OpenGLMemoryObjects.hpp>

#include <Algo/Any.hpp>
#include <Algo/Sort.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <RHI/Memory/RHITransientBatch.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

#include <Maths/Macros.hpp>

/**
 * File-local helpers
 */

namespace
{

inline Lumen::Bool SameBufferDesc ( const Lumen::RHI::FRHIBufferDesc &InFirst,
                                    const Lumen::RHI::FRHIBufferDesc &InSecond ) noexcept
{
    return InFirst.Size == InSecond.Size and InFirst.Usage == InSecond.Usage and InFirst.MemoryClass == InSecond.MemoryClass and
           InFirst.InitialState == InSecond.InitialState;
}

} // namespace

/**
 * Public
 */

Lumen::RHIOpenGL::FOpenGLBufferPool::FOpenGLBufferPool (
    TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> InCreator,
    const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLBufferPool>( InId ) ),
      Creator( static_cast<TFunction<TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError>( const RHI::FRHIBufferDesc & )> &&>(
          InCreator ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLBufferPool::~FOpenGLBufferPool () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLBufferPool );

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLBufferPool::Acquire ( const RHI::FRHIBufferDesc &InDesc ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    HAL::TLockGuard Lock( Mutex );
    for ( USize Index = 0U; Index < Available.size(); ++Index )
    {
        if ( SameBufferDesc( Available[Index]->GetDesc(), InDesc ) )
        {
            TSharedRef<RHI::IRHIBuffer> Reused = Available[Index];
            Available.erase( Available.begin() + static_cast<decltype( Available )::difference_type>( Index ) );
            return Reused;
        }
    }
    TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Created = Creator( InDesc );
    LUMEN_TRY_EXPECT_ERROR( Created );
    Owned.push_back( *Created );
    return *Created;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLBufferPool::Release ( TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                                        const RHI::FRHITimelineValue InCompletion ) noexcept
{
    HAL::TLockGuard Lock( Mutex );

    if ( not Algo::AnyOf( Owned, [&] ( const TSharedRef<RHI::IRHIBuffer> &OwnedBuffer )
    {
        return OwnedBuffer->GetId() == InBuffer->GetId();
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer pool does not own this buffer." ) );
    }
    if ( Algo::AnyOf( Pending, [&] ( const FPendingBuffer &PendingBuffer )
    {
        return PendingBuffer.Buffer->GetId() == InBuffer->GetId();
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer was already released to this pool." ) );
    }
    if ( Algo::AnyOf( Available, [&] ( const TSharedRef<RHI::IRHIBuffer> &AvailableBuffer )
    {
        return AvailableBuffer->GetId() == InBuffer->GetId();
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidResource( "Buffer was already released to this pool." ) );
    }

    FPendingBuffer PendingBuffer = { .Buffer     = static_cast<TSharedRef<RHI::IRHIBuffer> &&>( InBuffer ),
                                     .Completion = InCompletion };
    Pending.push_back( PendingBuffer );
    return {};
}

void Lumen::RHIOpenGL::FOpenGLBufferPool::AdvanceCompleted ( const RHI::FRHITimelineValue InCompleted ) noexcept
{
    HAL::TLockGuard Lock( Mutex );
    for ( USize Index = Pending.size(); Index > 0U; --Index )
    {
        const FPendingBuffer &Entry = Pending[Index - 1U];
        if ( RHI::IsRHICompletionReached( InCompleted, Entry.Completion ) )
        {
            Available.push_back( Entry.Buffer );
            Pending.erase( Pending.begin() + static_cast<decltype( Pending )::difference_type>( Index - 1U ) );
        }
    }
}

Lumen::USize Lumen::RHIOpenGL::FOpenGLBufferPool::GetPendingCount () const noexcept
{
    HAL::TLockGuard Lock( Mutex );
    return Pending.size();
}

Lumen::RHIOpenGL::FOpenGLTransientResourceAllocator::FOpenGLTransientResourceAllocator ( const RHI::FRHIObjectId InId,
                                                                                         const UInt64 InMinAlignment ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLTransientResourceAllocator>( InId ) ),
      MinAlignment( InMinAlignment > 1ULL ? InMinAlignment : 1ULL )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLTransientResourceAllocator::~FOpenGLTransientResourceAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLTransientResourceAllocator );

void Lumen::RHIOpenGL::FOpenGLTransientResourceAllocator::BeginBatch () noexcept
{
    HAL::TLockGuard Lock( Mutex );
    bInBatch = true;
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHITransientAllocation>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLTransientResourceAllocator::Allocate ( const RHI::FRHITransientBatch &InBatch ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InBatch.Validate() );
    HAL::TLockGuard Lock( Mutex );
    if ( not bInBatch )
    {
        return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Memory, "Transient batch is not open." ) );
    }

    const USize Count = InBatch.Requests.size();
    TVector<USize> Order;
    Order.reserve( Count );
    for ( USize Index = 0U; Index < Count; ++Index )
    {
        Order.push_back( Index );
    }
    Algo::Sort( Order, [&InBatch] ( const USize InLeft, const USize InRight ) noexcept
    {
        return InBatch.Requests[InLeft].Lifetime.Begin < InBatch.Requests[InRight].Lifetime.Begin;
    } );

    UInt64 SlotSize = 0ULL;
    for ( const RHI::FRHITransientDesc &Request : InBatch.Requests )
    {
        const UInt64 Alignment = Request.Requirements.Alignment > MinAlignment ? Request.Requirements.Alignment : MinAlignment;
        const TOptional<UInt64> Aligned = RHI::RHIAlignUp( Request.Requirements.Size, Alignment );
        if ( not Aligned )
        {
            return TUnexpected( RHI::MakeRHIInvalidMemory( "Transient requirement alignment overflowed." ) );
        }
        SlotSize = Maths::Max( *Aligned, SlotSize );
    }
    const TOptional<UInt64> AlignedSlot = RHI::RHIAlignUp( SlotSize, MinAlignment );
    if ( not AlignedSlot )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Transient slot alignment overflowed." ) );
    }
    const UInt64 Stride = *AlignedSlot;

    TVector<UInt64> ColorEnds;
    TVector<RHI::FRHITransientAllocation> Result;
    Result.reserve( Count );
    for ( const USize RequestIndex : Order )
    {
        const RHI::FRHITransientDesc &Request = InBatch.Requests[RequestIndex];
        USize Color                           = ColorEnds.size();
        for ( USize Candidate = 0U; Candidate < ColorEnds.size(); ++Candidate )
        {
            if ( ColorEnds[Candidate] <= Request.Lifetime.Begin )
            {
                Color = Candidate;
                break;
            }
        }
        if ( Color == ColorEnds.size() )
        {
            ColorEnds.push_back( 0ULL );
        }
        ColorEnds[Color] = Request.Lifetime.End;

        const TOptional<UInt64> Offset = RHI::RHICheckedMultiply( static_cast<UInt64>( Color ), Stride );
        if ( not Offset )
        {
            return TUnexpected( RHI::MakeRHIInvalidMemory( "Transient offset overflowed." ) );
        }
        RHI::FRHIAllocation Allocation{ .Id          = RHI::FRHIAllocationId{ NextId++ },
                                        .Offset      = *Offset,
                                        .Size        = Request.Requirements.Size,
                                        .Alignment   = Request.Requirements.Alignment,
                                        .MemoryClass = RHI::ERHIMemoryClass::DeviceLocal,
                                        .Heap        = nullptr };
        if ( Request.Requirements.PreferredMemoryClass.HasValue() )
        {
            Allocation.MemoryClass = *Request.Requirements.PreferredMemoryClass;
        }
        Result.push_back(
            RHI::FRHITransientAllocation{ .Allocation = Allocation, .ResourceId = {}, .Lifetime = Request.Lifetime } );
    }
    return Result;
}

void Lumen::RHIOpenGL::FOpenGLTransientResourceAllocator::EndBatch () noexcept
{
    HAL::TLockGuard Lock( Mutex );
    bInBatch = false;
}

Lumen::RHIOpenGL::FOpenGLDefragmenter::FOpenGLDefragmenter ( const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLDefragmenter>( InId ) )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLDefragmenter::~FOpenGLDefragmenter () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLDefragmenter );

Lumen::TExpected<Lumen::RHI::FRHIDefragPlan, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLDefragmenter::BuildPlan ( const TSpan<const RHI::FRHIAllocation> InAllocations ) noexcept
{
    if ( Algo::AnyOf( InAllocations, [] ( const RHI::FRHIAllocation &Allocation ) noexcept -> Bool
    {
        return not Allocation.IsValid() or Allocation.Heap == nullptr;
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag input contains an invalid allocation." ) );
    }
    return RHI::FRHIDefragPlan{};
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLDefragmenter::Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InPlan.Validate() );
    return TUnexpected(
        RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Memory, "Native OpenGL defragmenter moves are a later milestone." ) );
}
