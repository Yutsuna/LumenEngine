/**
 * @file RHIBufferPoolTests.cpp
 * @brief Tests compatible buffer reuse and completion-gated retirement.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIBufferPool.hpp>
#include <Memory/RHIBufferPool.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/RHITimeline.hpp>

namespace Lumen
{

LUMEN_TEST( RHIBufferPool, ReusesReleasedCompletedBuffer )
{
    Tests::FNullDevice Device;
    const TSharedRef<IRHIBufferPool> Pool = *Device.CreateBufferPool();
    const FRHIBufferDesc Desc{ .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource };
    const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer = Pool->Acquire( Desc );
    LUMEN_CHECK_TRUE( Buffer.HasValue() );
    const FRHIObjectId Id = ( *Buffer )->GetId();
    LUMEN_CHECK_TRUE( Pool->Release( *Buffer, {} ).HasValue() );
    const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Reused = Pool->Acquire( Desc );
    LUMEN_CHECK_TRUE( Reused.HasValue() );
    LUMEN_CHECK_EQ( ( *Reused )->GetId().GetValue(), Id.GetValue() );
}

LUMEN_TEST( RHIBufferPool, DoesNotReuseUntilCompletionIsReached )
{
    Tests::FNullDevice Device;
    FRHIBufferPool Pool( Device, FRHIObjectId{ 1ULL } );
    const FRHIBufferDesc Desc{ .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource };

    const TSharedRef<IRHIBuffer> First = *Pool.Acquire( Desc );
    const UInt64 FirstId               = First->GetId().GetValue();
    LUMEN_CHECK_TRUE( Pool.Release( First, FRHITimelineValue{ 7ULL } ).HasValue() );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    Pool.AdvanceCompleted( FRHITimelineValue{ 6ULL } );
    const TSharedRef<IRHIBuffer> Before = *Pool.Acquire( Desc );
    LUMEN_CHECK_NOT_EQ( Before->GetId().GetValue(), FirstId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    Pool.AdvanceCompleted( FRHITimelineValue{ 7ULL } );
    const TSharedRef<IRHIBuffer> After = *Pool.Acquire( Desc );
    LUMEN_CHECK_EQ( After->GetId().GetValue(), FirstId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 0U );
}

LUMEN_TEST( RHIBufferPool, DoesNotReuseWithDifferentMemoryClass )
{
    Tests::FNullDevice Device;
    FRHIBufferPool Pool( Device, FRHIObjectId{ 1ULL } );
    const FRHIBufferDesc Desc{ .Size         = 64ULL,
                               .Usage        = ERHIResourceUsage::ShaderResource,
                               .MemoryClass  = ERHIMemoryClass::DeviceLocal,
                               .InitialState = ERHIResourceState::Common };

    const TSharedRef<IRHIBuffer> Buffer = *Pool.Acquire( Desc );
    const UInt64 BufferId               = Buffer->GetId().GetValue();
    LUMEN_CHECK_TRUE( Pool.Release( Buffer, {} ).HasValue() );

    const FRHIBufferDesc UploadDesc{ .Size         = 64ULL,
                                     .Usage        = ERHIResourceUsage::ShaderResource,
                                     .MemoryClass  = ERHIMemoryClass::Upload,
                                     .InitialState = ERHIResourceState::Common };
    LUMEN_CHECK_NOT_EQ( ( *Pool.Acquire( UploadDesc ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    LUMEN_CHECK_EQ( ( *Pool.Acquire( Desc ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 0U );
}

LUMEN_TEST( RHIBufferPool, DoesNotReuseWithDifferentInitialState )
{
    Tests::FNullDevice Device;
    FRHIBufferPool Pool( Device, FRHIObjectId{ 1ULL } );
    const FRHIBufferDesc Desc{ .Size         = 64ULL,
                               .Usage        = ERHIResourceUsage::ShaderResource,
                               .MemoryClass  = ERHIMemoryClass::DeviceLocal,
                               .InitialState = ERHIResourceState::Common };

    const TSharedRef<IRHIBuffer> Buffer = *Pool.Acquire( Desc );
    const UInt64 BufferId               = Buffer->GetId().GetValue();
    LUMEN_CHECK_TRUE( Pool.Release( Buffer, {} ).HasValue() );

    const FRHIBufferDesc StateDesc{ .Size         = 64ULL,
                                    .Usage        = ERHIResourceUsage::ShaderResource,
                                    .MemoryClass  = ERHIMemoryClass::DeviceLocal,
                                    .InitialState = ERHIResourceState::CopyDestination };
    LUMEN_CHECK_NOT_EQ( ( *Pool.Acquire( StateDesc ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    LUMEN_CHECK_EQ( ( *Pool.Acquire( Desc ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 0U );
}

LUMEN_TEST( RHIBufferPool, RequiresMatchingSizeAndUsageForReuse )
{
    Tests::FNullDevice Device;
    FRHIBufferPool Pool( Device, FRHIObjectId{ 1ULL } );
    const FRHIBufferDesc Desc{ .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource };

    const TSharedRef<IRHIBuffer> Buffer = *Pool.Acquire( Desc );
    const UInt64 BufferId               = Buffer->GetId().GetValue();
    LUMEN_CHECK_TRUE( Pool.Release( Buffer, {} ).HasValue() );

    const FRHIBufferDesc DifferentSize{ .Size = 128ULL, .Usage = ERHIResourceUsage::ShaderResource };
    LUMEN_CHECK_NOT_EQ( ( *Pool.Acquire( DifferentSize ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    const FRHIBufferDesc DifferentUsage{ .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource };
    LUMEN_CHECK_NOT_EQ( ( *Pool.Acquire( DifferentUsage ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 1U );

    LUMEN_CHECK_EQ( ( *Pool.Acquire( Desc ) )->GetId().GetValue(), BufferId );
    LUMEN_CHECK_EQ( Pool.GetPendingCount(), 0U );
}

} // namespace Lumen
