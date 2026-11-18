/**
 * @file RHIFormatTests.cpp
 * @brief Tests format enum coverage.
 */

#include <RHI/Types/RHIFormatInfo.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
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

} // namespace RHI

} // namespace Lumen
