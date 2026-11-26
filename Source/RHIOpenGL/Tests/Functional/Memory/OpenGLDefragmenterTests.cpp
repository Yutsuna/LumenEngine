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
        GetRHIAllocationObserverRegistry().Unregister( Observer );
    }

} // namespace

} // namespace Lumen
