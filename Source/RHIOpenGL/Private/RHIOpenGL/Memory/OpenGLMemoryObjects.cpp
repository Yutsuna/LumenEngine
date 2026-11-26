/**
 * @file OpenGLMemoryObjects.cpp
 * @brief Implements OpenGL buffer recycling, transient placement and defrag planning.
 */

#include <RHIOpenGL/Memory/OpenGLMemoryObjects.hpp>

#include <Algo/Any.hpp>
#include <Algo/Find.hpp>
#include <Algo/Sort.hpp>

#include <HAL/LockGuard.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Memory/IRHIAllocationObserver.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <RHI/Memory/RHITransientBatch.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>
#include <RHI/Utilities/RHICheckedMath.hpp>

#include <Maths/Macros.hpp>
#include <Templates/Map.hpp>

#include <RHIOpenGL/Memory/OpenGLHeap.hpp>

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

/** @return The owning backend heap when the allocation is valid for this device. */
Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLHeap *> GetValidatedHeap ( const Lumen::RHIOpenGL::FOpenGLDeviceContext &InContext,
                                                                     const Lumen::RHI::FRHIAllocation &InAllocation ) noexcept
{
    if ( not InAllocation.IsValid() or InAllocation.Heap == nullptr )
    {
        return {};
    }
    const Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLHeap *> Heap =
        Lumen::RHI::TryRHIBackendCast<Lumen::RHIOpenGL::FOpenGLHeap>( *InAllocation.Heap );
    if ( not Heap or &( *Heap )->GetDeviceContext() != &InContext )
    {
        return {};
    }
    const Lumen::TOptional<Lumen::UInt64> End = Lumen::RHI::RHICheckedAdd( InAllocation.Offset, InAllocation.Size );
    if ( not End or *End > ( *Heap )->GetDesc().Size )
    {
        return {};
    }
    return Heap;
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
    const auto Found = Algo::FindIf( Available.begin(), Available.end(),
                                     [&InDesc] ( const TSharedRef<RHI::IRHIBuffer> &InBuffer ) noexcept -> Bool
    {
        return SameBufferDesc( InBuffer->GetDesc(), InDesc );
    } );
    if ( Found != Available.end() )
    {
        TSharedRef<RHI::IRHIBuffer> Reused = *Found;
        Available.erase( Found );
        return Reused;
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
        Allocation.MemoryClass = Request.Requirements.PreferredMemoryClass.ValueOr( Allocation.MemoryClass );
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

Lumen::RHIOpenGL::FOpenGLDefragmenter::FOpenGLDefragmenter ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                             const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLDefragmenter>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) )
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
    if ( InAllocations.empty() )
    {
        return RHI::FRHIDefragPlan{};
    }

    TMap<const RHI::IRHIHeap *, TVector<RHI::FRHIAllocation>> Groups;
    for ( const RHI::FRHIAllocation &Allocation : InAllocations )
    {
        if ( not GetValidatedHeap( *Context, Allocation ) )
        {
            return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag input contains a foreign or invalid allocation." ) );
        }
        Groups[Allocation.Heap].push_back( Allocation );
    }

    RHI::FRHIDefragPlan Plan;
    for ( auto &Group : Groups )
    {
        TVector<RHI::FRHIAllocation> &Sorted = Group.second;
        Algo::Sort( Sorted, [] ( const RHI::FRHIAllocation &InLeft, const RHI::FRHIAllocation &InRight ) noexcept -> Bool
        {
            return InLeft.Offset < InRight.Offset;
        } );

        Bool bHasPrevious  = false;
        UInt64 PreviousEnd = 0ULL;
        UInt64 LastEnd     = 0ULL;
        for ( const RHI::FRHIAllocation &Allocation : Sorted )
        {
            const TOptional<UInt64> End = RHI::RHICheckedAdd( Allocation.Offset, Allocation.Size );
            if ( not End )
            {
                return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag allocation range overflows." ) );
            }
            if ( bHasPrevious and Allocation.Offset < PreviousEnd )
            {
                return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag input allocations overlap." ) );
            }
            PreviousEnd  = *End;
            LastEnd      = *End;
            bHasPrevious = true;
        }

        UInt64 Cursor = 0ULL;
        for ( const RHI::FRHIAllocation &Allocation : Sorted )
        {
            const UInt64 Alignment          = Allocation.Alignment > 1ULL ? Allocation.Alignment : 1ULL;
            const TOptional<UInt64> Aligned = RHI::RHIAlignUp( Cursor, Alignment );
            if ( not Aligned )
            {
                return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag alignment overflowed." ) );
            }
            const TOptional<UInt64> End = RHI::RHICheckedAdd( *Aligned, Allocation.Size );
            if ( not End )
            {
                return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag destination range overflowed." ) );
            }
            RHI::FRHIAllocation Destination = Allocation;
            Destination.Offset              = *Aligned;
            if ( Destination.Offset != Allocation.Offset )
            {
                Plan.Moves.push_back( RHI::FRHIDefragMove{
                    .Source = Allocation, .Destination = Destination, .Completion = RHI::FRHITimelineValue{ 1ULL } } );
            }
            Cursor = *End;
        }
        if ( LastEnd > Cursor )
        {
            Plan.ReclaimedBytes += LastEnd - Cursor;
        }
    }

    LUMEN_TRY_EXPECT_ERROR( Plan.Validate() );
    return Plan;
}

Lumen::RHI::FRHIExpected Lumen::RHIOpenGL::FOpenGLDefragmenter::Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept
{
    LUMEN_TRY_OPTIONAL_ERROR( Context->GetUnavailableError( "Defragmenter::Execute" ) );
    LUMEN_TRY_EXPECT_ERROR( InPlan.Validate() );

    Bool bNeedsScratch = false;
    UInt64 ScratchSize = 0ULL;
    for ( const RHI::FRHIDefragMove &Move : InPlan.Moves )
    {
        if ( not RHI::IsRHICompletionReached( InPlan.ObservedCompletion, Move.Completion ) )
        {
            return TUnexpected( RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Memory,
                                                          "Defragmentation source is still in use by the GPU." ) );
        }
        const TOptional<FOpenGLHeap *> Source      = GetValidatedHeap( *Context, Move.Source );
        const TOptional<FOpenGLHeap *> Destination = GetValidatedHeap( *Context, Move.Destination );
        if ( not Source or not Destination )
        {
            return TUnexpected( RHI::MakeRHIInvalidMemory( "Defrag move references a foreign or invalid heap." ) );
        }
        if ( RHI::RHIRangesOverlap( Move.Source.Offset, Move.Source.Size, Move.Destination.Offset, Move.Destination.Size ) )
        {
            bNeedsScratch = true;
            ScratchSize   = Maths::Max( ScratchSize, Move.Source.Size );
        }
    }

    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    if ( Dispatch.GetError == nullptr or
         ( not InPlan.Moves.empty() and
           ( Dispatch.CopyNamedBufferSubData == nullptr or
             ( bNeedsScratch and ( Dispatch.CreateBuffers == nullptr or Dispatch.DeleteBuffers == nullptr or
                                   Dispatch.NamedBufferStorage == nullptr ) ) ) ) )
    {
        return TUnexpected( RHI::MakeRHIBackendFailure(
            RHI::ERHIErrorCondition::Memory, "OpenGL dispatch table is missing a required defragmentation entry point." ) );
    }

    GLuint Scratch                 = 0U;
    GLenum NativeError             = GlNoError;
    Bool bScratchReady             = true;
    const RHI::FRHIExpected Posted = Context->GetExecutor()->ExecuteOnGLThread( [&] () noexcept
    {
        if ( bNeedsScratch )
        {
            Dispatch.CreateBuffers( 1, &Scratch );
            Dispatch.NamedBufferStorage( Scratch, static_cast<GLsizeiptr>( ScratchSize ), nullptr, GlDynamicStorageBit );
            if ( Scratch == 0U )
            {
                bScratchReady = false;
                return;
            }
        }

        for ( const RHI::FRHIDefragMove &Move : InPlan.Moves )
        {
            const GLuint SourceBuffer      = ( *GetValidatedHeap( *Context, Move.Source ) )->GetBuffer();
            const GLuint DestinationBuffer = ( *GetValidatedHeap( *Context, Move.Destination ) )->GetBuffer();
            const GLsizeiptr Size          = static_cast<GLsizeiptr>( Move.Source.Size );
            if ( bNeedsScratch and
                 RHI::RHIRangesOverlap( Move.Source.Offset, Move.Source.Size, Move.Destination.Offset, Move.Destination.Size ) )
            {
                Dispatch.CopyNamedBufferSubData( SourceBuffer, Scratch, static_cast<GLintptr>( Move.Source.Offset ), 0LL, Size );
                Dispatch.CopyNamedBufferSubData( Scratch, DestinationBuffer, 0LL,
                                                 static_cast<GLintptr>( Move.Destination.Offset ), Size );
            }
            else
            {
                Dispatch.CopyNamedBufferSubData( SourceBuffer, DestinationBuffer, static_cast<GLintptr>( Move.Source.Offset ),
                                                 static_cast<GLintptr>( Move.Destination.Offset ), Size );
            }
        }
        NativeError = Dispatch.GetError();
        if ( Scratch != 0U )
        {
            Dispatch.DeleteBuffers( 1, &Scratch );
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Posted );
    if ( not bScratchReady )
    {
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory,
                                                        "OpenGL could not allocate a defragmentation scratch buffer." ) );
    }
    if ( NativeError != GlNoError )
    {
        return TUnexpected(
            RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Memory, "OpenGL reported an error during defragmentation." ) );
    }

    for ( const RHI::FRHIDefragMove &Move : InPlan.Moves )
    {
        RHI::GetRHIAllocationObserverRegistry().NotifyMoved( Move.Source, Move.Destination );
    }
    return {};
}
