/**
 * @file OpenGLDefragmenterTests.cpp
 * @brief Functional coverage for native heap moves in M4.4.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/IRHIAllocationObserver.hpp>
#include <RHI/Memory/IRHIDefragmenter.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Memory/RHIDefragPlan.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>

#include <Templates/Array.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    using namespace RHI;

    class FMoveObserver final : public IRHIAllocationObserver
    {
    public:

        void OnAllocationCreated ( const FRHIAllocation & ) noexcept override
        {
            ++Created;
        }
        void OnAllocationReleased ( const FRHIAllocation & ) noexcept override
        {
            ++Released;
        }
        void OnAllocationMoved ( const FRHIAllocation &InBefore, const FRHIAllocation &InAfter ) noexcept override
        {
            ++Moved;
            if ( InAfter.Offset < InBefore.Offset )
            {
                ++Forward;
            }
        }

        UInt32 Created  = 0U;
        UInt32 Released = 0U;
        UInt32 Moved    = 0U;
        UInt32 Forward  = 0U;
    };

    void Fill ( const TSpan<Byte> InSpan, const Byte InValue ) noexcept
    {
        for ( Byte &Value : InSpan )
        {
            Value = InValue;
        }
    }

    Bool Matches ( const TSpan<Byte> InSpan, const Byte InValue ) noexcept
    {
        for ( const Byte Value : InSpan )
        {
            if ( Value != InValue )
            {
                return false;
            }
        }
        return true;
    }

    LUMEN_TEST( OpenGLDefragmenter, BuildsAndExecutesCompactedHeapMoves )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        const TExpected<TSharedRef<IRHIHeap>, FRHIError> Heap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( Heap.HasValue() );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> First =
            ( *Device )->CreatePlacedBuffer( { .Size = 512ULL, .MemoryClass = ERHIMemoryClass::Upload }, *Heap,
                                             FRHIAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                                             .Offset      = 0ULL,
                                                             .Size        = 512ULL,
                                                             .Alignment   = 256ULL,
                                                             .MemoryClass = ERHIMemoryClass::Upload,
                                                             .Heap        = Heap->Get() } );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Second =
            ( *Device )->CreatePlacedBuffer( { .Size = 512ULL, .MemoryClass = ERHIMemoryClass::Upload }, *Heap,
                                             FRHIAllocation{ .Id          = FRHIAllocationId{ 2ULL },
                                                             .Offset      = 2048ULL,
                                                             .Size        = 512ULL,
                                                             .Alignment   = 256ULL,
                                                             .MemoryClass = ERHIMemoryClass::Upload,
                                                             .Heap        = Heap->Get() } );
        LUMEN_CHECK_TRUE( First.HasValue() );
        LUMEN_CHECK_TRUE( Second.HasValue() );

        const TExpected<TSpan<Byte>, FRHIError> FirstSpan = ( *Heap )->Map( 0ULL, 512ULL );
        LUMEN_CHECK_TRUE( FirstSpan.HasValue() );
        Fill( *FirstSpan, Byte{ 0x11U } );
        ( *Heap )->Unmap();
        const TExpected<TSpan<Byte>, FRHIError> SecondSpan = ( *Heap )->Map( 2048ULL, 512ULL );
        LUMEN_CHECK_TRUE( SecondSpan.HasValue() );
        Fill( *SecondSpan, Byte{ 0x22U } );
        ( *Heap )->Unmap();

        const TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> Defragmenter = ( *Device )->CreateDefragmenter();
        LUMEN_CHECK_TRUE( Defragmenter.HasValue() );

        TArray<FRHIAllocation, 2U> Input                = { FRHIAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                                                            .Offset      = 0ULL,
                                                                            .Size        = 512ULL,
                                                                            .Alignment   = 256ULL,
                                                                            .MemoryClass = ERHIMemoryClass::Upload,
                                                                            .Heap        = Heap->Get() },
                                                            FRHIAllocation{ .Id          = FRHIAllocationId{ 2ULL },
                                                                            .Offset      = 2048ULL,
                                                                            .Size        = 512ULL,
                                                                            .Alignment   = 256ULL,
                                                                            .MemoryClass = ERHIMemoryClass::Upload,
                                                                            .Heap        = Heap->Get() } };
        const TExpected<FRHIDefragPlan, FRHIError> Plan = ( *Defragmenter )->BuildPlan( Input );
        LUMEN_CHECK_TRUE( Plan.HasValue() );
        LUMEN_CHECK_EQ( Plan->Moves.size(), 1U );
        LUMEN_CHECK_TRUE( Plan->ReclaimedBytes > 0ULL );

        FMoveObserver Observer;
        GetRHIAllocationObserverRegistry().Register( Observer );
        FRHIDefragPlan Executable     = *Plan;
        Executable.ObservedCompletion = FRHITimelineValue{ 1ULL };
        LUMEN_CHECK_TRUE( ( *Defragmenter )->Execute( Executable ).HasValue() );
        LUMEN_CHECK_EQ( Observer.Moved, 1U );
        LUMEN_CHECK_EQ( Observer.Forward, 1U );
        GetRHIAllocationObserverRegistry().Unregister( Observer );

        const TExpected<TSpan<Byte>, FRHIError> FirstAfter = ( *Heap )->Map( 0ULL, 512ULL );
        LUMEN_CHECK_TRUE( FirstAfter.HasValue() );
        LUMEN_CHECK_TRUE( Matches( *FirstAfter, Byte{ 0x11U } ) );
        ( *Heap )->Unmap();
        const TExpected<TSpan<Byte>, FRHIError> SecondAfter = ( *Heap )->Map( 512ULL, 512ULL );
        LUMEN_CHECK_TRUE( SecondAfter.HasValue() );
        LUMEN_CHECK_TRUE( Matches( *SecondAfter, Byte{ 0x22U } ) );
        ( *Heap )->Unmap();
    }

    LUMEN_TEST( OpenGLDefragmenter, BuildsAndExecutesMultiHeapPlan )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );

        const TExpected<TSharedRef<IRHIHeap>, FRHIError> FirstHeap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } );
        const TExpected<TSharedRef<IRHIHeap>, FRHIError> SecondHeap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( FirstHeap.HasValue() );
        LUMEN_CHECK_TRUE( SecondHeap.HasValue() );

        const FRHIAllocation FirstAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                              .Offset      = 2048ULL,
                                              .Size        = 512ULL,
                                              .Alignment   = 256ULL,
                                              .MemoryClass = ERHIMemoryClass::Upload,
                                              .Heap        = FirstHeap->Get() };
        const FRHIAllocation SecondAllocation{ .Id          = FRHIAllocationId{ 2ULL },
                                               .Offset      = 2048ULL,
                                               .Size        = 512ULL,
                                               .Alignment   = 256ULL,
                                               .MemoryClass = ERHIMemoryClass::Upload,
                                               .Heap        = SecondHeap->Get() };
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> First = ( *Device )->CreatePlacedBuffer(
            { .Size = 512ULL, .MemoryClass = ERHIMemoryClass::Upload }, *FirstHeap, FirstAllocation );
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Second = ( *Device )->CreatePlacedBuffer(
            { .Size = 512ULL, .MemoryClass = ERHIMemoryClass::Upload }, *SecondHeap, SecondAllocation );
        LUMEN_CHECK_TRUE( First.HasValue() );
        LUMEN_CHECK_TRUE( Second.HasValue() );

        const TExpected<TSpan<Byte>, FRHIError> FirstSpan = ( *FirstHeap )->Map( 2048ULL, 512ULL );
        LUMEN_CHECK_TRUE( FirstSpan.HasValue() );
        Fill( *FirstSpan, Byte{ 0x11U } );
        ( *FirstHeap )->Unmap();
        const TExpected<TSpan<Byte>, FRHIError> SecondSpan = ( *SecondHeap )->Map( 2048ULL, 512ULL );
        LUMEN_CHECK_TRUE( SecondSpan.HasValue() );
        Fill( *SecondSpan, Byte{ 0x22U } );
        ( *SecondHeap )->Unmap();

        const TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> Defragmenter = ( *Device )->CreateDefragmenter();
        LUMEN_CHECK_TRUE( Defragmenter.HasValue() );

        TArray<FRHIAllocation, 2U> Input                = { FirstAllocation, SecondAllocation };
        const TExpected<FRHIDefragPlan, FRHIError> Plan = ( *Defragmenter )->BuildPlan( Input );
        LUMEN_CHECK_TRUE( Plan.HasValue() );
        LUMEN_CHECK_EQ( Plan->Moves.size(), 2U );
        LUMEN_CHECK_EQ( Plan->Moves[0U].Destination.Offset, 0ULL );
        LUMEN_CHECK_EQ( Plan->Moves[1U].Destination.Offset, 0ULL );
        LUMEN_CHECK_TRUE( Plan->Moves[0U].Source.Heap == Plan->Moves[0U].Destination.Heap );
        LUMEN_CHECK_TRUE( Plan->Moves[1U].Source.Heap == Plan->Moves[1U].Destination.Heap );
        LUMEN_CHECK_TRUE( Plan->Moves[0U].Destination.Heap != Plan->Moves[1U].Destination.Heap );
        LUMEN_CHECK_TRUE( Plan->ReclaimedBytes > 0ULL );
        LUMEN_CHECK_TRUE( Plan->Validate().HasValue() );

        FMoveObserver Observer;
        GetRHIAllocationObserverRegistry().Register( Observer );
        FRHIDefragPlan Executable     = *Plan;
        Executable.ObservedCompletion = FRHITimelineValue{ 1ULL };
        LUMEN_CHECK_TRUE( ( *Defragmenter )->Execute( Executable ).HasValue() );
        LUMEN_CHECK_EQ( Observer.Moved, 2U );
        LUMEN_CHECK_EQ( Observer.Forward, 2U );
        GetRHIAllocationObserverRegistry().Unregister( Observer );

        const TExpected<TSpan<Byte>, FRHIError> FirstAfter = ( *FirstHeap )->Map( 0ULL, 512ULL );
        LUMEN_CHECK_TRUE( FirstAfter.HasValue() );
        LUMEN_CHECK_TRUE( Matches( *FirstAfter, Byte{ 0x11U } ) );
        ( *FirstHeap )->Unmap();
        const TExpected<TSpan<Byte>, FRHIError> SecondAfter = ( *SecondHeap )->Map( 0ULL, 512ULL );
        LUMEN_CHECK_TRUE( SecondAfter.HasValue() );
        LUMEN_CHECK_TRUE( Matches( *SecondAfter, Byte{ 0x22U } ) );
        ( *SecondHeap )->Unmap();
    }

    LUMEN_TEST( OpenGLDefragmenter, ExecutesOverlappingMoveUsingScratch )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TExpected<TSharedRef<IRHIHeap>, FRHIError> Heap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( Heap.HasValue() );

        const FRHIAllocation Allocation{ .Id          = FRHIAllocationId{ 1ULL },
                                         .Offset      = 256ULL,
                                         .Size        = 512ULL,
                                         .Alignment   = 256ULL,
                                         .MemoryClass = ERHIMemoryClass::Upload,
                                         .Heap        = Heap->Get() };
        const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
            ( *Device )->CreatePlacedBuffer( { .Size = 512ULL, .MemoryClass = ERHIMemoryClass::Upload }, *Heap, Allocation );
        LUMEN_CHECK_TRUE( Buffer.HasValue() );

        const TExpected<TSpan<Byte>, FRHIError> SourceSpan = ( *Heap )->Map( 256ULL, 512ULL );
        LUMEN_CHECK_TRUE( SourceSpan.HasValue() );
        Fill( *SourceSpan, Byte{ 0x33U } );
        ( *Heap )->Unmap();

        const TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> Defragmenter = ( *Device )->CreateDefragmenter();
        LUMEN_CHECK_TRUE( Defragmenter.HasValue() );
        TArray<FRHIAllocation, 1U> Input                = { Allocation };
        const TExpected<FRHIDefragPlan, FRHIError> Plan = ( *Defragmenter )->BuildPlan( Input );
        LUMEN_CHECK_TRUE( Plan.HasValue() );
        LUMEN_CHECK_EQ( Plan->Moves.size(), 1U );
        LUMEN_CHECK_EQ( Plan->Moves[0U].Destination.Offset, 0ULL );

        FRHIDefragPlan Executable     = *Plan;
        Executable.ObservedCompletion = FRHITimelineValue{ 1ULL };
        LUMEN_CHECK_TRUE( ( *Defragmenter )->Execute( Executable ).HasValue() );

        const TExpected<TSpan<Byte>, FRHIError> DestinationSpan = ( *Heap )->Map( 0ULL, 512ULL );
        LUMEN_CHECK_TRUE( DestinationSpan.HasValue() );
        LUMEN_CHECK_TRUE( Matches( *DestinationSpan, Byte{ 0x33U } ) );
        ( *Heap )->Unmap();
    }

    LUMEN_TEST( OpenGLDefragmenter, RejectsInvalidInputsAndUnreachedCompletion )
    {
        const TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = RHI::Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        const TExpected<TSharedRef<IRHIHeap>, FRHIError> Heap =
            ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } );
        LUMEN_CHECK_TRUE( Heap.HasValue() );
        const TExpected<TSharedRef<IRHIDefragmenter>, FRHIError> Defragmenter = ( *Device )->CreateDefragmenter();
        LUMEN_CHECK_TRUE( Defragmenter.HasValue() );

        TArray<FRHIAllocation, 1U> Transient = { FRHIAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                                                 .Offset      = 0ULL,
                                                                 .Size        = 128ULL,
                                                                 .Alignment   = 256ULL,
                                                                 .MemoryClass = ERHIMemoryClass::DeviceLocal,
                                                                 .Heap        = nullptr } };
        LUMEN_CHECK_HAS_ERROR( ( *Defragmenter )->BuildPlan( Transient ) );

        TArray<FRHIAllocation, 2U> Overlapping = { FRHIAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                                                   .Offset      = 0ULL,
                                                                   .Size        = 512ULL,
                                                                   .Alignment   = 256ULL,
                                                                   .MemoryClass = ERHIMemoryClass::Upload,
                                                                   .Heap        = Heap->Get() },
                                                   FRHIAllocation{ .Id          = FRHIAllocationId{ 2ULL },
                                                                   .Offset      = 256ULL,
                                                                   .Size        = 512ULL,
                                                                   .Alignment   = 256ULL,
                                                                   .MemoryClass = ERHIMemoryClass::Upload,
                                                                   .Heap        = Heap->Get() } };
        LUMEN_CHECK_HAS_ERROR( ( *Defragmenter )->BuildPlan( Overlapping ) );

        FRHIDefragPlan Plan;
        Plan.Moves.push_back( FRHIDefragMove{ .Source      = FRHIAllocation{ .Id          = FRHIAllocationId{ 1ULL },
                                                                             .Offset      = 0ULL,
                                                                             .Size        = 128ULL,
                                                                             .Alignment   = 256ULL,
                                                                             .MemoryClass = ERHIMemoryClass::Upload,
                                                                             .Heap        = Heap->Get() },
                                              .Destination = FRHIAllocation{ .Id          = FRHIAllocationId{ 2ULL },
                                                                             .Offset      = 256ULL,
                                                                             .Size        = 128ULL,
                                                                             .Alignment   = 256ULL,
                                                                             .MemoryClass = ERHIMemoryClass::Upload,
                                                                             .Heap        = Heap->Get() },
                                              .Completion  = FRHITimelineValue{ 1ULL } } );

        FMoveObserver Observer;
        GetRHIAllocationObserverRegistry().Register( Observer );
        const FRHIExpected NotReady = ( *Defragmenter )->Execute( Plan );
        LUMEN_CHECK_HAS_ERROR( NotReady );
        LUMEN_CHECK_EQ( NotReady.Error().Code, ERHIErrorCode::InvalidState );
        LUMEN_CHECK_EQ( Observer.Moved, 0U );

        const FRHIAllocation MoveSource{ .Id          = FRHIAllocationId{ 5ULL },
                                         .Offset      = 512ULL,
                                         .Size        = 128ULL,
                                         .Alignment   = 256ULL,
                                         .MemoryClass = ERHIMemoryClass::Upload,
                                         .Heap        = Heap->Get() };
        const FRHIAllocation FirstDestination{ .Id          = FRHIAllocationId{ 6ULL },
                                               .Offset      = 0ULL,
                                               .Size        = 128ULL,
                                               .Alignment   = 256ULL,
                                               .MemoryClass = ERHIMemoryClass::Upload,
                                               .Heap        = Heap->Get() };
        const FRHIAllocation SecondDestination{ .Id          = FRHIAllocationId{ 7ULL },
                                                .Offset      = 0ULL,
                                                .Size        = 128ULL,
                                                .Alignment   = 256ULL,
                                                .MemoryClass = ERHIMemoryClass::Upload,
                                                .Heap        = Heap->Get() };
        FRHIDefragPlan OverlappingDestinations;
        OverlappingDestinations.ObservedCompletion = FRHITimelineValue{ 1ULL };
        OverlappingDestinations.Moves.push_back(
            FRHIDefragMove{ .Source = MoveSource, .Destination = FirstDestination, .Completion = FRHITimelineValue{ 1ULL } } );
        OverlappingDestinations.Moves.push_back(
            FRHIDefragMove{ .Source = MoveSource, .Destination = SecondDestination, .Completion = FRHITimelineValue{ 1ULL } } );
        const FRHIExpected Rejected = ( *Defragmenter )->Execute( OverlappingDestinations );
        LUMEN_CHECK_HAS_ERROR( Rejected );
        LUMEN_CHECK_EQ( Rejected.Error().Code, ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Rejected.Error().Condition, ERHIErrorCondition::Memory );
        LUMEN_CHECK_EQ( Observer.Moved, 0U );
        GetRHIAllocationObserverRegistry().Unregister( Observer );
    }

} // namespace

} // namespace Lumen
