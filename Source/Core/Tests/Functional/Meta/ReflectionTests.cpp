/**
 * @file ReflectionTests.cpp
 * @brief Unit tests for static-reflection enum name helpers.
 */

#include <Meta/Reflection.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Test Data
 */

namespace
{

    /** NOLINTNEXTLINE */
    enum class ETestColor : UInt8
    {
        Red,
        Green,
        Blue = 7U,
    };

    /** NOLINTNEXTLINE */
    enum ETestPlain
    {
        First,
        Second,
    };

#if LUMEN_HAS_REFLECTION
    static_assert( Meta::EnumName( ETestColor::Green ) == "Green" );
    static_assert( Meta::EnumName( ETestPlain::Second ) == "Second" );
    static_assert( Meta::EnumQualifiedName( ETestColor::Green ) == "ETestColor::Green" );
    static_assert( Meta::EnumQualifiedName( ETestColor::Blue ) == "ETestColor::Blue" );
    static_assert( Meta::EnumQualifiedName( ETestPlain::Second ) == "ETestPlain::Second" );
    static_assert( Meta::EnumName( static_cast<ETestColor>( 99U ) ) == "<unknown>" );
    static_assert( Meta::EnumQualifiedName( static_cast<ETestColor>( 99U ) ) == "<unknown>" );
#endif

    static_assert( noexcept( Meta::EnumName( ETestColor::Red ) ) );
    static_assert( noexcept( Meta::EnumQualifiedName( ETestColor::Red ) ) );

    LUMEN_TEST( Reflection_EnumName, ReturnsUnqualifiedEnumeratorName )
    {
#if LUMEN_HAS_REFLECTION
        LUMEN_CHECK_TRUE( Meta::EnumName( ETestColor::Red ) == "Red" );
        LUMEN_CHECK_TRUE( Meta::EnumName( ETestPlain::Second ) == "Second" );
#else
        LUMEN_CHECK_TRUE( Meta::EnumName( ETestColor::Red ) == "<unknown>" );
#endif
    }

    LUMEN_TEST( Reflection_EnumQualifiedName, ReturnsQualifiedEnumeratorName )
    {
#if LUMEN_HAS_REFLECTION
        LUMEN_CHECK_TRUE( Meta::EnumQualifiedName( ETestColor::Red ) == "ETestColor::Red" );
        LUMEN_CHECK_TRUE( Meta::EnumQualifiedName( ETestPlain::Second ) == "ETestPlain::Second" );
#else
        LUMEN_CHECK_TRUE( Meta::EnumQualifiedName( ETestColor::Red ) == "<unknown>" );
#endif
    }

    LUMEN_TEST( Reflection_EnumName, UnknownFallback )
    {
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) — deliberate out-of-range value for the unknown-enum path
        LUMEN_CHECK_TRUE( Meta::EnumName( static_cast<ETestColor>( 123U ) ) == "<unknown>" );
        // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange) — deliberate out-of-range value for the unknown-enum path
        LUMEN_CHECK_TRUE( Meta::EnumQualifiedName( static_cast<ETestColor>( 123U ) ) == "<unknown>" );
    }

} // namespace

} // namespace Lumen
