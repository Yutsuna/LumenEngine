/**
 * @file RHIShaderFormatTests.cpp
 * @brief Tests shader format enumeration bounds.
 */

#include <RHI/Shaders/RHIShaderFormat.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIShaderFormat, AcceptsOnlyDefinedValues )
        {
            for ( UInt8 Value = 0U; Value < static_cast<UInt8>( ERHIShaderFormat::Count ); ++Value )
            {
                LUMEN_CHECK_TRUE( IsKnownRHIShaderFormat( static_cast<ERHIShaderFormat>( Value ) ) );
            }
            LUMEN_CHECK_FALSE( IsKnownRHIShaderFormat( ERHIShaderFormat::Count ) );
            LUMEN_CHECK_FALSE( IsKnownRHIShaderFormat( static_cast<ERHIShaderFormat>( 255U ) ) );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
