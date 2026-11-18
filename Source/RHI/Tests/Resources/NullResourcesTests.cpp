/**
 * @file NullResourcesTests.cpp
 * @brief Tests Null resource storage and views.
 */

#include "Device/NullDevice.hpp"
#include <Resources/IRHIBuffer.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( NullResources, BufferMappingWritesOwnedStorage )
{
    Tests::FNullDevice Device;
    const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
        Device.CreateBuffer( { .Size = 32ULL, .Usage = ERHIResourceUsage::CopySource, .MemoryClass = ERHIMemoryClass::Upload } );
    const TExpected<TSpan<Byte>, FRHIError> Mapping = ( *Buffer )->Map( 0ULL, 32ULL );
    LUMEN_CHECK_TRUE( Mapping.HasValue() );
    ( *Mapping )[3] = 0x42U;
    LUMEN_CHECK_EQ( ( *Mapping )[3], 0x42U );
}

} // namespace Lumen
