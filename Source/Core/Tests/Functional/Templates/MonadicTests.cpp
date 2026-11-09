/**
 * @file MonadicTests.cpp
 * @brief Functional tests for the FMonadic mixin: Transform, AndThen, OrElse, Inspect, ValueOr.
 */

#include <Core/CoreDefinitions.hpp>

#include <Concepts/SameAs.hpp>
#include <String/String.hpp>

#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>

#include <Tests/UnitTest.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    enum class EMonadicTestError : UInt8
    {
        ErrorA = 1,
        ErrorB = 2,
    };

    struct FIntRecorder
    {
        Int32 Captured = 0;
        Int32 Calls    = 0;

        void operator()( const Int32 InValue ) noexcept
        {
            ++Calls;
            Captured = InValue;
        }
    };

    [[nodiscard]] FString IntToFString ( Int32 Value ) noexcept
    {
        return FString{ std::to_string( Value ) };
    }

    [[nodiscard]] TExpected<Int32, EMonadicTestError> Return42 () noexcept
    {
        return { 42 };
    }

    void NoOpInspect ( Int32 LUMEN_UNUSED NoOp ) noexcept
    {
        /**
         *
         */
    }

    /**
     * @brief Compile-time invariants: result types of the monadic operations.
     */
    static_assert( Concepts::CSameAs<decltype( std::declval<TExpected<Int32, EMonadicTestError> &>().Transform( &IntToFString ) ),
                                     TExpected<FString, EMonadicTestError>> );
    static_assert( Concepts::CSameAs<decltype( std::declval<TExpected<void, EMonadicTestError> &>().AndThen( &Return42 ) ),
                                     TExpected<Int32, EMonadicTestError>> );
    static_assert( std::is_lvalue_reference_v<decltype( std::declval<TOptional<Int32> &>().Inspect( &NoOpInspect ) )> );
    static_assert( not std::is_reference_v<decltype( std::declval<TOptional<Int32> &&>().Inspect( &NoOpInspect ) )> );

} // namespace

/**
 * Transform
 */

LUMEN_TEST( Monadic_Transform, ExpectedValue )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 21 );
    constexpr auto Result = Expected.Transform( [] ( const Int32 Value ) noexcept
    {
        return Value * 2;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_Transform, ExpectedErrorPropagates )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    constexpr auto Result = Expected.Transform( [] ( const Int32 Value ) noexcept
    {
        return Value * 2;
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EMonadicTestError::ErrorA );
}

LUMEN_TEST( Monadic_Transform, ExpectedTypeChange )
{
    const TExpected<Int32, EMonadicTestError> Expected( 42 );
    auto Result = Expected.Transform( [] ( const Int32 Value )
    {
        return FString( std::to_string( Value ) );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, "42" );
}

LUMEN_TEST( Monadic_Transform, ExpectedChained )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 1 );
    constexpr auto Result = Expected
                                .Transform(
                                    [] ( const Int32 V ) noexcept
    {
        return V + 1;
    } )
                                .Transform( [] ( const Int32 V ) noexcept
    {
        return V * 10;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 20 );
}

LUMEN_TEST( Monadic_Transform, OptionalValue )
{
    TOptional<Int32> Optional( 21 );
    auto Result = Optional.Transform( [] ( const Int32 Value ) noexcept
    {
        return Value * 2;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_Transform, OptionalEmptySkipsFunction )
{
    TOptional<Int32> Optional;
    Int32 CallCount = 0;
    auto Result     = Optional.Transform( [&CallCount] ( const Int32 Value ) noexcept
    {
        ++CallCount;
        return Value * 2;
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( CallCount, 0 );
}

LUMEN_TEST( Monadic_Transform, OptionalTypeChange )
{
    TOptional<Int32> Optional( 7 );
    auto Result = Optional.Transform( [] ( const Int32 Value )
    {
        return FString( std::to_string( Value ) );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, "7" );
}

LUMEN_TEST( Monadic_Transform, OptionalChained )
{
    TOptional<Int32> Optional( 1 );
    auto Result = Optional
                      .Transform(
                          [] ( const Int32 V ) noexcept
    {
        return V + 1;
    } )
                      .Transform( [] ( const Int32 V ) noexcept
    {
        return V * 10;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 20 );
}

LUMEN_TEST( Monadic_Transform, RvalueOptional )
{
    auto Result = TOptional<Int32>( 5 ).Transform( [] ( const Int32 Value ) noexcept
    {
        return Value + 1;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 6 );
}

/**
 * AndThen
 */

LUMEN_TEST( Monadic_AndThen, ExpectedValue )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 21 );
    constexpr auto Result = Expected.AndThen( [] ( const Int32 Value ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( Value * 2 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_AndThen, ExpectedOuterErrorPropagates )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    constexpr auto Result = Expected.AndThen( [] ( const Int32 Value ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( Value * 2 );
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EMonadicTestError::ErrorA );
}

LUMEN_TEST( Monadic_AndThen, ExpectedInnerErrorPropagates )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 21 );
    constexpr auto Result = Expected.AndThen( [] ( const Int32 ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( FUnexpect{}, EMonadicTestError::ErrorB );
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EMonadicTestError::ErrorB );
}

LUMEN_TEST( Monadic_AndThen, ExpectedTypeChange )
{
    const TExpected<Int32, EMonadicTestError> Expected( 42 );
    auto Result = Expected.AndThen( [] ( const Int32 Value )
    {
        return TExpected<FString, EMonadicTestError>( FString( std::to_string( Value ) ) );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, "42" );
}

LUMEN_TEST( Monadic_AndThen, ExpectedChained )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 1 );
    constexpr auto Result = Expected
                                .AndThen(
                                    [] ( const Int32 V ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( V + 1 );
    } )
                                .AndThen( [] ( const Int32 V ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( V * 10 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 20 );
}

LUMEN_TEST( Monadic_AndThen, ExpectedToVoid )
{
    const TExpected<Int32, EMonadicTestError> Expected( 42 );
    auto Result = Expected.AndThen( [] ( const Int32 )
    {
        return TExpected<void, EMonadicTestError>();
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
}

LUMEN_TEST( Monadic_AndThen, OptionalValue )
{
    TOptional<Int32> Optional( 21 );
    auto Result = Optional.AndThen( [] ( const Int32 Value ) noexcept
    {
        return TOptional<Int32>( Value * 2 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_AndThen, OptionalEmptySkipsFunction )
{
    TOptional<Int32> Optional;
    Int32 CallCount = 0;
    auto Result     = Optional.AndThen( [&CallCount] ( const Int32 Value ) noexcept
    {
        ++CallCount;
        return TOptional<Int32>( Value );
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( CallCount, 0 );
}

LUMEN_TEST( Monadic_AndThen, OptionalInnerEmpty )
{
    TOptional<Int32> Optional( 21 );
    auto Result = Optional.AndThen( [] ( const Int32 ) noexcept
    {
        return TOptional<Int32>();
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
}

/**
 * AndThen on TExpected<void, E>
 */

LUMEN_TEST( Monadic_AndThenVoid, SuccessInvokesFunction )
{
    const TExpected<void, EMonadicTestError> Expected;
    auto Result = Expected.AndThen( []
    {
        return TExpected<Int32, EMonadicTestError>( 42 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_AndThenVoid, ErrorPropagates )
{
    const TExpected<void, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    auto Result = Expected.AndThen( []
    {
        return TExpected<Int32, EMonadicTestError>( 42 );
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EMonadicTestError::ErrorA );
}

LUMEN_TEST( Monadic_AndThenVoid, ChainWithValue )
{
    const TExpected<void, EMonadicTestError> Expected;
    auto Result = Expected
                      .AndThen(
                          []
    {
        return TExpected<Int32, EMonadicTestError>( 1 );
    } )
                      .AndThen( [] ( const Int32 V )
    {
        return TExpected<Int32, EMonadicTestError>( V + 1 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 2 );
}

/**
 * OrElse
 */

LUMEN_TEST( Monadic_OrElse, ExpectedValueSkipsFunction )
{
    TExpected<Int32, EMonadicTestError> Expected( 42 );
    Int32 CallCount = 0;
    auto Result     = std::move( Expected )
                      .OrElse( [&CallCount] ( const EMonadicTestError ) noexcept
    {
        ++CallCount;
        return TExpected<Int32, EMonadicTestError>( 0 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
    LUMEN_CHECK_EQ( CallCount, 0 );
}

LUMEN_TEST( Monadic_OrElse, ExpectedErrorInvokesFunction )
{
    TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    auto Result = std::move( Expected )
                      .OrElse( [] ( const EMonadicTestError InError ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( std::to_underlying( InError ) * 10 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 10 );
}

LUMEN_TEST( Monadic_OrElse, ExpectedErrorCanProduceNewError )
{
    TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    auto Result = std::move( Expected )
                      .OrElse( [] ( const EMonadicTestError ) noexcept
    {
        return TExpected<Int32, EMonadicTestError>( FUnexpect{}, EMonadicTestError::ErrorB );
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Result.Error(), EMonadicTestError::ErrorB );
}

LUMEN_TEST( Monadic_OrElse, ExpectedVoidRecovers )
{
    TExpected<void, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    auto Result = std::move( Expected )
                      .OrElse( [] ( const EMonadicTestError ) noexcept
    {
        return TExpected<void, EMonadicTestError>();
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
}

LUMEN_TEST( Monadic_OrElse, OptionalValueSkipsFunction )
{
    TOptional<Int32> Optional( 42 );
    Int32 CallCount = 0;
    auto Result     = std::move( Optional )
                      .OrElse( [&CallCount] () noexcept
    {
        ++CallCount;
        return TOptional<Int32>( 0 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
    LUMEN_CHECK_EQ( CallCount, 0 );
}

LUMEN_TEST( Monadic_OrElse, OptionalEmptyInvokesFunction )
{
    TOptional<Int32> Optional;
    auto Result = std::move( Optional )
                      .OrElse( [] () noexcept
    {
        return TOptional<Int32>( 99 );
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 99 );
}

/**
 * Inspect
 */

LUMEN_TEST( Monadic_Inspect, ExpectedValueInvokesCallback )
{
    TExpected<Int32, EMonadicTestError> Expected( 42 );
    FIntRecorder Recorder;
    auto Result = std::move( Expected ).Inspect( Recorder );

    LUMEN_CHECK_EQ( Recorder.Calls, 1 );
    LUMEN_CHECK_EQ( Recorder.Captured, 42 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Monadic_Inspect, ExpectedErrorSkipsCallback )
{
    TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    FIntRecorder Recorder;
    auto Result = std::move( Expected ).Inspect( Recorder );

    LUMEN_CHECK_EQ( Recorder.Calls, 0 );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Monadic_Inspect, OptionalValueInvokesCallback )
{
    TOptional<Int32> Optional( 42 );
    FIntRecorder Recorder;
    auto Result = std::move( Optional ).Inspect( Recorder );

    LUMEN_CHECK_EQ( Recorder.Calls, 1 );
    LUMEN_CHECK_EQ( Recorder.Captured, 42 );
    LUMEN_CHECK_TRUE( Result.HasValue() );
}

LUMEN_TEST( Monadic_Inspect, OptionalEmptySkipsCallback )
{
    TOptional<Int32> Optional;
    FIntRecorder Recorder;
    auto Result = std::move( Optional ).Inspect( Recorder );

    LUMEN_CHECK_EQ( Recorder.Calls, 0 );
    LUMEN_CHECK_FALSE( Result.HasValue() );
}

LUMEN_TEST( Monadic_Inspect, LvalueReturnsSameReference )
{
    TOptional<Int32> Optional( 42 );
    auto &Returned = Optional.Inspect( [] ( const Int32 ) noexcept
    {
    } );

    LUMEN_CHECK_EQ( std::addressof( Returned ), std::addressof( Optional ) );
}

LUMEN_TEST( Monadic_Inspect, Chained )
{
    TOptional<Int32> Optional( 42 );
    Int32 Total = 0;
    Optional
        .Inspect(
            [&Total] ( const Int32 V ) noexcept
    {
        Total += V;
    } )
        .Inspect( [&Total] ( const Int32 V ) noexcept
    {
        Total += V;
    } );

    LUMEN_CHECK_EQ( Total, 84 );
}

/**
 * ValueOr
 */

LUMEN_TEST( Monadic_ValueOr, ExpectedValue )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( 42 );
    constexpr Int32 Result = Expected.ValueOr( 0 );

    LUMEN_CHECK_EQ( Result, 42 );
}

LUMEN_TEST( Monadic_ValueOr, ExpectedError )
{
    constexpr TExpected<Int32, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    constexpr Int32 Result = Expected.ValueOr( 99 );

    LUMEN_CHECK_EQ( Result, 99 );
}

LUMEN_TEST( Monadic_ValueOr, OptionalValue )
{
    const TOptional<Int32> Optional( 42 );
    LUMEN_CHECK_EQ( Optional.ValueOr( 0 ), 42 );
}

LUMEN_TEST( Monadic_ValueOr, OptionalEmpty )
{
    const TOptional<Int32> Optional;
    LUMEN_CHECK_EQ( Optional.ValueOr( 99 ), 99 );
}

LUMEN_TEST( Monadic_ValueOr, ConvertingFallback )
{
    constexpr TExpected<Int64, EMonadicTestError> Expected( FUnexpect{}, EMonadicTestError::ErrorA );
    constexpr Int64 Result = Expected.ValueOr( 42 );

    LUMEN_CHECK_EQ( Result, 42 );
}

LUMEN_TEST( Monadic_ValueOr, RvalueCarriesValue )
{
    TExpected<FString, EMonadicTestError> Expected( FString( "payload" ) );
    FString Result = std::move( Expected ).ValueOr( FString( "fallback" ) );

    LUMEN_CHECK_EQ( Result, "payload" );
}

} // namespace Lumen
