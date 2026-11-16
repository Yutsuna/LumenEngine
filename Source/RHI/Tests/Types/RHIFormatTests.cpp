/**
 * @file RHIFormatTests.cpp
 * @brief Tests format enum coverage.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIFormatInfo.hpp>

namespace Lumen
{

LUMEN_TEST( RHIFormat, EveryEnumHasMatchingMetadata )
{
    constexpr USize Count = static_cast<USize>( ERHIFormat::Count );
    for ( USize Index = 0U; Index < Count; ++Index )
    {
        const ERHIFormat Format = static_cast<ERHIFormat>( Index );
        LUMEN_CHECK_TRUE( GetRHIFormatInfo( Format ).Format == Format );
    }

    LUMEN_CHECK_FALSE( TryGetRHIFormatInfo( ERHIFormat::Count ).HasValue() );
    LUMEN_CHECK_EQ( GetRHIFormatInfo( ERHIFormat::Count ).Format, ERHIFormat::Unknown );
}

} // namespace Lumen
