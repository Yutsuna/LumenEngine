/// @file ExpectedTests.cpp
/// @brief Unit tests for TExpected class

#include <String/String.hpp>
#include <Templates/Expected.hpp>
#include <Tests/UnitTest.hpp>

namespace
{

enum class EExpectedTestError
{
    ErrorA = 100,
    ErrorB = 200,
};

}

namespace Lumen
{

LUMEN_TEST( Expected_Constructor, ValueLValue )
{
    constexpr Int32 SourceValue = 42;
    constexpr TExpected<Int32, EExpectedTestError> Exp( SourceValue );

    LUMEN_CHECK_TRUE( Exp.HasValue() );
    LUMEN_CHECK_EQ( *Exp, 42 );
}

LUMEN_TEST( Expected_Constructor, ValueRValue )
{
    FString SourceText = "HelloLumen";
    const TExpected<FString, EExpectedTestError> Exp( std::move( SourceText ) );

    LUMEN_CHECK_TRUE( Exp.HasValue() );
    LUMEN_CHECK_EQ( *Exp, "HelloLumen" );
    LUMEN_CHECK_TRUE( SourceText.empty() );
}

LUMEN_TEST( Expected_Constructor, ErrorLValue )
{
    constexpr EExpectedTestError SourceError = EExpectedTestError::ErrorA;
    constexpr TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, SourceError );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_Constructor, ErrorRValue )
{
    FString ErrorText = "CustomError";
    const TExpected<Int32, FString> Exp( FUnexpect{}, std::move( ErrorText ) );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), "CustomError" );
    LUMEN_CHECK_TRUE( ErrorText.empty() );
}

LUMEN_TEST( Expected_Constructor, FromUnexpectedSameType )
{
    constexpr TUnexpected<EExpectedTestError> Unexp( EExpectedTestError::ErrorB );
    constexpr TExpected<Int32, EExpectedTestError> Exp( Unexp );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorB );
}

} // namespace Lumen