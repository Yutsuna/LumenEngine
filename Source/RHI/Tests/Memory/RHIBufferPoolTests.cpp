/**
 * @file RHIBufferPoolTests.cpp
 * @brief Tests compatible buffer reuse.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIBufferPool.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

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

} // namespace Lumen
