/**
 * @file VulkanMemoryObjects.cpp
 * @brief Implements Vulkan buffer recycling, transient placement and compaction planning.
 */

#include <RHIVulkan/Memory/VulkanMemoryObjects.hpp>

#include <RHIVulkan/Memory/VulkanHeap.hpp>
#include <RHIVulkan/Resources/VulkanBuffer.hpp>
#include <RHIVulkan/Types/VulkanObject.hpp>

#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Memory/RHITransientBatch.hpp>
#include <RHI/Memory/RHITransientDesc.hpp>
#include <RHI/Types/RHITimeline.hpp>
#include <RHI/Utilities/RHIAlignment.hpp>

/**
 * Public
 */

Lumen::RHIVulkan::FVulkanBufferPool::FVulkanBufferPool ( TSharedRef<FVulkanDeviceContext> InContext,
                                                         RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanBufferPool>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) )
{
    /* */
}

Lumen::RHIVulkan::FVulkanBufferPool::~FVulkanBufferPool () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanBufferPool )

Lumen::RHI::FRHIObjectId Lumen::RHIVulkan::FVulkanBufferPool::NextId () noexcept
{
    return Identifiers.Next<RHI::FRHIObjectIdTag>();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIBuffer>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanBufferPool::Acquire ( const RHI::FRHIBufferDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    for ( USize Index = 0U; Index < Released.size(); )
    {
        if ( RHI::IsRHICompletionReached( CurrentCompleted, Released[Index].Completion ) )
        {
            Free.push_back( static_cast<FEntry &&>( Released[Index] ) );
            Released.erase( Released.begin() + static_cast<ISize>( Index ) );
        }
        else
        {
            ++Index;
        }
    }
    for ( USize Index = 0U; Index < Free.size(); ++Index )
    {
        const RHI::FRHIBufferDesc &Candidate = Free[Index].Buffer->GetDesc();
        if ( Candidate.Size == InDesc.Size and Candidate.Usage == InDesc.Usage and Candidate.MemoryClass == InDesc.MemoryClass and
             Candidate.InitialState == InDesc.InitialState )
        {
            TSharedRef<RHI::IRHIBuffer> Buffer = Free[Index].Buffer;
            Free.erase( Free.begin() + static_cast<ISize>( Index ) );
            return Buffer;
        }
    }
    TExpected<TSharedRef<FVulkanBuffer>, RHI::FRHIError> Buffer = FVulkanBuffer::Create( Context, InDesc, NextId() );
    if ( not Buffer )
    {
        return TUnexpected( Buffer.Error() );
    }
    return *Buffer;
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanBufferPool::Release ( TSharedRef<RHI::IRHIBuffer> InBuffer,
                                                                        RHI::FRHITimelineValue InCompletion ) noexcept
{
    Released.push_back( FEntry{ .Buffer = static_cast<TSharedRef<RHI::IRHIBuffer> &&>( InBuffer ), .Completion = InCompletion } );
    return {};
}

void Lumen::RHIVulkan::FVulkanBufferPool::AdvanceCompleted ( const RHI::FRHITimelineValue InCompleted ) noexcept
{
    if ( InCompleted > CurrentCompleted )
    {
        CurrentCompleted = InCompleted;
    }
}

Lumen::USize Lumen::RHIVulkan::FVulkanBufferPool::GetPendingCount () const noexcept
{
    return Released.size();
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanTransientResourceAllocator>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanTransientResourceAllocator::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                                              RHI::FRHIObjectId InId,
                                                              RHI::FRHIObjectId InHeapId ) noexcept
{
    constexpr UInt64 ArenaSize = 64ULL * 1024ULL * 1024ULL;
    TExpected<TSharedRef<FVulkanHeap>, RHI::FRHIError> Heap =
        FVulkanHeap::Create( InContext, { .Size = ArenaSize, .MemoryClass = RHI::ERHIMemoryClass::Upload }, InHeapId );
    if ( not Heap )
    {
        return TUnexpected( Heap.Error() );
    }
    return MakeShared<FVulkanTransientResourceAllocator>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ),
                                                          static_cast<TSharedRef<FVulkanHeap> &&>( *Heap ), InId );
}

Lumen::RHIVulkan::FVulkanTransientResourceAllocator::FVulkanTransientResourceAllocator (
    TSharedRef<FVulkanDeviceContext> InContext, TSharedRef<FVulkanHeap> InHeap, RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanTransientResourceAllocator>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ),
      Heap( static_cast<TSharedRef<FVulkanHeap> &&>( InHeap ) ), Allocator( InHeap->GetDesc().Size )
{
    /* */
}

Lumen::RHIVulkan::FVulkanTransientResourceAllocator::~FVulkanTransientResourceAllocator () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanTransientResourceAllocator )

void Lumen::RHIVulkan::FVulkanTransientResourceAllocator::BeginBatch () noexcept
{
    bInBatch = true;
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHITransientAllocation>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanTransientResourceAllocator::Allocate ( const RHI::FRHITransientBatch &InBatch ) noexcept
{
    if ( not bInBatch )
    {
        return TUnexpected(
            RHI::MakeRHIInvalidState( RHI::ERHIErrorCondition::Memory, "Transient allocation requires an active batch." ) );
    }
    LUMEN_EXPECT( Validation, InBatch.Validate() );

    TVector<RHI::FRHITransientAllocation> Result;
    Result.reserve( InBatch.Requests.size() );
    for ( const RHI::FRHITransientDesc &Request : InBatch.Requests )
    {
        constexpr RHI::ERHIMemoryClass ArenaClass = RHI::ERHIMemoryClass::Upload;
        if ( Request.Requirements.PreferredMemoryClass.HasValue() and *Request.Requirements.PreferredMemoryClass != ArenaClass )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Memory,
                                                         "Transient arena only provides upload-class memory." ) );
        }
        LUMEN_EXPECT( Allocation, Allocator.Allocate( { .Size        = Request.Requirements.Size,
                                                        .Alignment   = Request.Requirements.Alignment,
                                                        .MemoryClass = ArenaClass } ) );
        Allocation.Heap = Heap.Get();
        Result.push_back(
            RHI::FRHITransientAllocation{ .Allocation = Allocation, .ResourceId = {}, .Lifetime = Request.Lifetime } );
    }
    return Result;
}

void Lumen::RHIVulkan::FVulkanTransientResourceAllocator::EndBatch () noexcept
{
    bInBatch = false;
}

Lumen::RHIVulkan::FVulkanDefragmenter::FVulkanDefragmenter ( RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanDefragmenter>( InId ) )
{
    /* */
}

Lumen::RHIVulkan::FVulkanDefragmenter::~FVulkanDefragmenter () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanDefragmenter )

Lumen::TExpected<Lumen::RHI::FRHIDefragPlan, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanDefragmenter::BuildPlan ( const TSpan<const RHI::FRHIAllocation> InAllocations ) noexcept
{
    RHI::FRHIDefragPlan Plan;
    UInt64 DestinationOffset = 0ULL;
    for ( USize Index = 0U; Index < InAllocations.size(); ++Index )
    {
        const RHI::FRHIAllocation &Source = InAllocations[Index];
        if ( not Source.IsValid() )
        {
            return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                             "Defragmentation input contains an invalid allocation." ) );
        }
        for ( USize Other = 0U; Other < Index; ++Other )
        {
            if ( Source.Heap == InAllocations[Other].Heap and
                 RHI::RHIRangesOverlap( Source.Offset, Source.Size, InAllocations[Other].Offset, InAllocations[Other].Size ) )
            {
                return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Memory,
                                                                 "Defragmentation input contains overlapping allocations." ) );
            }
        }

        RHI::FRHIAllocation Destination = Source;
        Destination.Offset              = DestinationOffset;
        DestinationOffset += Source.Size;
        Plan.Moves.push_back( RHI::FRHIDefragMove{ .Source = Source, .Destination = Destination } );
        Plan.ReclaimedBytes += Source.Size;
    }
    return Plan;
}

Lumen::RHI::FRHIExpected Lumen::RHIVulkan::FVulkanDefragmenter::Execute ( const RHI::FRHIDefragPlan &InPlan ) noexcept
{
    LUMEN_EXPECT( Validation, InPlan.Validate() );
    return TUnexpected(
        RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Memory,
                                 "Native defragmentation moves require Vulkan copy commands from the V4 command milestone." ) );
}
