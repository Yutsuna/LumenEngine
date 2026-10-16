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

LUMEN_TEST( Expected_Copy, Value )
{
    constexpr TExpected<Int32, EExpectedTestError> Original( 42 );
    constexpr TExpected<Int32, EExpectedTestError> Copy( Original );

    LUMEN_CHECK_TRUE( Copy.HasValue() );
    LUMEN_CHECK_EQ( *Copy, 42 );
}

LUMEN_TEST( Expected_Copy, Error )
{
    constexpr TExpected<Int32, EExpectedTestError> Original( FUnexpect{}, EExpectedTestError::ErrorA );
    constexpr TExpected<Int32, EExpectedTestError> Copy( Original );

    LUMEN_CHECK_FALSE( Copy.HasValue() );
    LUMEN_CHECK_EQ( Copy.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_Move, Value )
{
    FString SourceText = "MoveValue";
    TExpected<FString, EExpectedTestError> Original( std::move( SourceText ) );

    TExpected<FString, EExpectedTestError> Moved( std::move( Original ) );

    LUMEN_CHECK_TRUE( Moved.HasValue() );
    LUMEN_CHECK_EQ( *Moved, "MoveValue" );
}

LUMEN_TEST( Expected_Move, Error )
{
    FString ErrorText = "MoveError";
    TExpected<Int32, FString> Original( FUnexpect{}, std::move( ErrorText ) );

    TExpected<Int32, FString> Moved( std::move( Original ) );

    LUMEN_CHECK_FALSE( Moved.HasValue() );
    LUMEN_CHECK_EQ( Moved.Error(), "MoveError" );
}

LUMEN_TEST( Expected_CopyAssignment, ValueToValue )
{
    TExpected<Int32, EExpectedTestError> A( 10 );
    TExpected<Int32, EExpectedTestError> B( 20 );

    B = A;

    LUMEN_CHECK_TRUE( B.HasValue() );
    LUMEN_CHECK_EQ( *B, 10 );
}

LUMEN_TEST( Expected_CopyAssignment, ErrorToError )
{
    TExpected<Int32, EExpectedTestError> A( FUnexpect{}, EExpectedTestError::ErrorA );
    TExpected<Int32, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorB );

    B = A;

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_CopyAssignment, ValueToError )
{
    TExpected<Int32, EExpectedTestError> A( 10 );
    TExpected<Int32, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorA );

    B = A;

    LUMEN_CHECK_TRUE( B.HasValue() );
    LUMEN_CHECK_EQ( *B, 10 );
}

LUMEN_TEST( Expected_CopyAssignment, ErrorToValue )
{
    TExpected<Int32, EExpectedTestError> A( FUnexpect{}, EExpectedTestError::ErrorA );
    TExpected<Int32, EExpectedTestError> B( 10 );

    B = A;

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_CopyAssignment, SelfAssignment )
{
    TExpected<Int32, EExpectedTestError> A( 42 );
    A = A;

    LUMEN_CHECK_TRUE( A.HasValue() );
    LUMEN_CHECK_EQ( *A, 42 );
}

LUMEN_TEST( Expected_MoveAssignment, ValueToValue )
{
    TExpected<FString, EExpectedTestError> A( FString( "Moved" ) );
    TExpected<FString, EExpectedTestError> B( FString( "Old" ) );

    B = std::move( A );

    LUMEN_CHECK_TRUE( B.HasValue() );
    LUMEN_CHECK_EQ( *B, "Moved" );
}

LUMEN_TEST( Expected_MoveAssignment, ErrorToError )
{
    TExpected<Int32, FString> A( FUnexpect{}, FString( "ErrA" ) );
    TExpected<Int32, FString> B( FUnexpect{}, FString( "ErrB" ) );

    B = std::move( A );

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), "ErrA" );
}

LUMEN_TEST( Expected_MoveAssignment, ValueToError )
{
    TExpected<FString, EExpectedTestError> A( FString( "Moved" ) );
    TExpected<FString, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorA );

    B = std::move( A );

    LUMEN_CHECK_TRUE( B.HasValue() );
    LUMEN_CHECK_EQ( *B, "Moved" );
}

LUMEN_TEST( Expected_MoveAssignment, ErrorToValue )
{
    TExpected<Int32, FString> A( FUnexpect{}, FString( "ErrA" ) );
    TExpected<Int32, FString> B( 10 );

    B = std::move( A );

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), "ErrA" );
}

LUMEN_TEST( Expected_Operators, Dereference )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 42 );
    constexpr Int32 Value = *Exp;

    LUMEN_CHECK_EQ( Value, 42 );
}

LUMEN_TEST( Expected_Operators, Arrow )
{
    TExpected<FString, EExpectedTestError> Exp( FString( "Arrow" ) );

    LUMEN_CHECK_EQ( static_cast<Int32>( Exp->size() ), 5 );
}

LUMEN_TEST( Expected_Operators, ErrorAccessor )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_Operators, ExplicitBool )
{
    constexpr TExpected<Int32, EExpectedTestError> ExpValue( 42 );
    constexpr TExpected<Int32, EExpectedTestError> ExpError( FUnexpect{}, EExpectedTestError::ErrorA );

    LUMEN_CHECK_TRUE( static_cast<Bool>( ExpValue ) );
    LUMEN_CHECK_FALSE( static_cast<Bool>( ExpError ) );
}

LUMEN_TEST( Expected_Transform, Value )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 21 );

    constexpr auto Result = Exp.Transform( [] ( Int32 Value ) { return Value * 2; } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Expected_Transform, ErrorPropagates )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    constexpr auto Result = Exp.Transform( [] ( Int32 Value ) { return Value * 2; } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_Transform, TypeChange )
{
    TExpected<Int32, EExpectedTestError> Exp( 42 );

    auto Result = Exp.Transform( [] ( Int32 Value ) { return FString( std::to_string( Value ) ); } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, "42" );
}

LUMEN_TEST( Expected_AndThen, Value )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 21 );

    constexpr auto Result = Exp.AndThen( [] ( Int32 Value ) { return TExpected<Int32, EExpectedTestError>( Value * 2 ); } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Expected_AndThen, InnerError )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 21 );

    constexpr auto Result =
        Exp.AndThen( [] ( Int32 ) { return TExpected<Int32, EExpectedTestError>( FUnexpect{}, EExpectedTestError::ErrorA ); } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_AndThen, ErrorPropagates )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    constexpr auto Result = Exp.AndThen( [] ( Int32 Value ) { return TExpected<Int32, EExpectedTestError>( Value * 2 ); } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( Expected_AndThen, Chained )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 1 );

    constexpr auto Result = Exp.AndThen( [] ( Int32 V ) { return TExpected<Int32, EExpectedTestError>( V + 1 ); } )
                                .AndThen( [] ( Int32 V ) { return TExpected<Int32, EExpectedTestError>( V * 2 ); } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 4 );
}

LUMEN_TEST( Expected_OrElse, ValueSkipsFunc )
{
    TExpected<Int32, EExpectedTestError> Exp( 42 );

    auto Result = std::move( Exp ).OrElse( [] ( EExpectedTestError ) { return TExpected<Int32, EExpectedTestError>( 0 ); } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Expected_OrElse, ErrorCallsFunc )
{
    TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    auto Result = std::move( Exp ).OrElse( [] ( EExpectedTestError ) { return TExpected<Int32, EExpectedTestError>( 99 ); } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 99 );
}

LUMEN_TEST( Expected_OrElse, ErrorPropagatesFromFunc )
{
    TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    auto Result = std::move( Exp ).OrElse(
        [] ( EExpectedTestError ) { return TExpected<Int32, EExpectedTestError>( FUnexpect{}, EExpectedTestError::ErrorB ); } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EExpectedTestError::ErrorB );
}

LUMEN_TEST( Expected_Inspect, ValueCallsFunc )
{
    TExpected<Int32, EExpectedTestError> Exp( 42 );
    Int32 Captured = 0;

    auto Result = std::move( Exp ).Inspect( [&] ( Int32 Value ) { Captured = Value; } );

    LUMEN_CHECK_EQ( Captured, 42 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
}

LUMEN_TEST( Expected_Inspect, ErrorSkipsFunc )
{
    TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );
    Int32 Captured = 0;

    auto Result = std::move( Exp ).Inspect( [&] ( Int32 Value ) { Captured = Value; } );

    LUMEN_CHECK_EQ( Captured, 0 );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Expected_ValueOr, ValueReturnsValue )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( 42 );

    constexpr Int32 Result = Exp.ValueOr( 0 );

    LUMEN_CHECK_EQ( Result, 42 );
}

LUMEN_TEST( Expected_ValueOr, ErrorReturnsFallback )
{
    constexpr TExpected<Int32, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    constexpr Int32 Result = Exp.ValueOr( 99 );

    LUMEN_CHECK_EQ( Result, 99 );
}

LUMEN_TEST( Expected_Rebind, TypeAlias )
{
    static_assert(
        std::is_same_v<TExpected<Int32, EExpectedTestError>::Rebind<FString>, TExpected<FString, EExpectedTestError>> );
}

/*---------------------------------------------------------------------------------------------------------------------------------------*/
/*  TExpected<void, ErrorType> specialization tests */
/*---------------------------------------------------------------------------------------------------------------------------------------*/

LUMEN_TEST( ExpectedVoid_Constructor, Default )
{
    TExpected<void, EExpectedTestError> Exp;

    LUMEN_CHECK_TRUE( Exp.HasValue() );
}

LUMEN_TEST( ExpectedVoid_Constructor, ErrorLValue )
{
    constexpr TExpected<void, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( ExpectedVoid_Constructor, ErrorRValue )
{
    FString ErrorText = "VoidError";
    const TExpected<void, FString> Exp( FUnexpect{}, std::move( ErrorText ) );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), "VoidError" );
    LUMEN_CHECK_TRUE( ErrorText.empty() );
}

LUMEN_TEST( ExpectedVoid_Constructor, FromUnexpected )
{
    constexpr TUnexpected<EExpectedTestError> Unexp( EExpectedTestError::ErrorB );
    constexpr TExpected<void, EExpectedTestError> Exp( Unexp );

    LUMEN_CHECK_FALSE( Exp.HasValue() );
    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorB );
}

LUMEN_TEST( ExpectedVoid_Copy, Value )
{
    TExpected<void, EExpectedTestError> Original;
    TExpected<void, EExpectedTestError> Copy( Original );

    LUMEN_CHECK_TRUE( Copy.HasValue() );
}

LUMEN_TEST( ExpectedVoid_Copy, Error )
{
    constexpr TExpected<void, EExpectedTestError> Original( FUnexpect{}, EExpectedTestError::ErrorA );
    constexpr TExpected<void, EExpectedTestError> Copy( Original );

    LUMEN_CHECK_FALSE( Copy.HasValue() );
    LUMEN_CHECK_EQ( Copy.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( ExpectedVoid_Move, Value )
{
    TExpected<void, EExpectedTestError> Original;
    TExpected<void, EExpectedTestError> Moved( std::move( Original ) );

    LUMEN_CHECK_TRUE( Moved.HasValue() );
}

LUMEN_TEST( ExpectedVoid_Move, Error )
{
    TExpected<void, FString> Original( FUnexpect{}, FString( "VoidMoveError" ) );
    TExpected<void, FString> Moved( std::move( Original ) );

    LUMEN_CHECK_FALSE( Moved.HasValue() );
    LUMEN_CHECK_EQ( Moved.Error(), "VoidMoveError" );
}

LUMEN_TEST( ExpectedVoid_CopyAssignment, ValueToValue )
{
    TExpected<void, EExpectedTestError> A;
    TExpected<void, EExpectedTestError> B;

    B = A;

    LUMEN_CHECK_TRUE( B.HasValue() );
}

LUMEN_TEST( ExpectedVoid_CopyAssignment, ErrorToError )
{
    TExpected<void, EExpectedTestError> A( FUnexpect{}, EExpectedTestError::ErrorA );
    TExpected<void, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorB );

    B = A;

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( ExpectedVoid_CopyAssignment, ValueToError )
{
    TExpected<void, EExpectedTestError> A;
    TExpected<void, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorA );

    B = A;

    LUMEN_CHECK_TRUE( B.HasValue() );
}

LUMEN_TEST( ExpectedVoid_CopyAssignment, ErrorToValue )
{
    TExpected<void, EExpectedTestError> A( FUnexpect{}, EExpectedTestError::ErrorA );
    TExpected<void, EExpectedTestError> B;

    B = A;

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( ExpectedVoid_CopyAssignment, SelfAssignment )
{
    TExpected<void, EExpectedTestError> A;
    A = A;

    LUMEN_CHECK_TRUE( A.HasValue() );
}

LUMEN_TEST( ExpectedVoid_MoveAssignment, ErrorToError )
{
    TExpected<void, FString> A( FUnexpect{}, FString( "MovedErr" ) );
    TExpected<void, FString> B( FUnexpect{}, FString( "OldErr" ) );

    B = std::move( A );

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), "MovedErr" );
}

LUMEN_TEST( ExpectedVoid_MoveAssignment, ValueToError )
{
    TExpected<void, EExpectedTestError> A;
    TExpected<void, EExpectedTestError> B( FUnexpect{}, EExpectedTestError::ErrorA );

    B = std::move( A );

    LUMEN_CHECK_TRUE( B.HasValue() );
}

LUMEN_TEST( ExpectedVoid_MoveAssignment, ErrorToValue )
{
    TExpected<void, FString> A( FUnexpect{}, FString( "VoidErr" ) );
    TExpected<void, FString> B;

    B = std::move( A );

    LUMEN_CHECK_FALSE( B.HasValue() );
    LUMEN_CHECK_EQ( B.Error(), "VoidErr" );
}

LUMEN_TEST( ExpectedVoid_Operators, Dereference )
{
    TExpected<void, EExpectedTestError> Exp;

    *Exp;
}

LUMEN_TEST( ExpectedVoid_Operators, ErrorAccessor )
{
    constexpr TExpected<void, EExpectedTestError> Exp( FUnexpect{}, EExpectedTestError::ErrorA );

    LUMEN_CHECK_EQ( Exp.Error(), EExpectedTestError::ErrorA );
}

LUMEN_TEST( ExpectedVoid_Operators, ExplicitBool )
{
    TExpected<void, EExpectedTestError> ExpValue;
    constexpr TExpected<void, EExpectedTestError> ExpError( FUnexpect{}, EExpectedTestError::ErrorA );

    LUMEN_CHECK_TRUE( static_cast<Bool>( ExpValue ) );
    LUMEN_CHECK_FALSE( static_cast<Bool>( ExpError ) );
}

LUMEN_TEST( ExpectedVoid_Rebind, TypeAlias )
{
    static_assert( std::is_same_v<TExpected<void, EExpectedTestError>::Rebind<Int32>, TExpected<Int32, EExpectedTestError>> );
}

} // namespace Lumen