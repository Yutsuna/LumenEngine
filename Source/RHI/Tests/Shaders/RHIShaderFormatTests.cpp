/**
 * @file RHIShaderFormatTests.cpp
 * @brief Tests shader format enumeration bounds.
 */

#include <Shaders/RHIShaderFormat.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{

    LUMEN_TEST( RHIShaderFormat, AcceptsOnlyDefinedValues )
    {
        for ( UInt8 Value = 0U; Value <= static_cast<UInt8>( ERHIShaderFormat::HLSL ); ++Value )
        {
            LUMEN_CHECK_TRUE( IsKnownRHIShaderFormat( static_cast<ERHIShaderFormat>( Value ) ) );
        }
        LUMEN_CHECK_FALSE( IsKnownRHIShaderFormat( static_cast<ERHIShaderFormat>( 6U ) ) );
        LUMEN_CHECK_FALSE( IsKnownRHIShaderFormat( static_cast<ERHIShaderFormat>( 255U ) ) );
    }

} // namespace
} // namespace Lumen
