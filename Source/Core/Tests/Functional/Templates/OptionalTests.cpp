/**
 * @file OptionalTests.cpp
 * @brief Unit tests for TOptional lifetime and monadic operations.
 */

#include <Templates/Optional.hpp>
#include <Tests/UnitTest.hpp>

#include <concepts>
#include <utility>

namespace Lumen
{

namespace
{

    struct FLifetimeState
    {
        Int32 Alive       = 0;
        Int32 Constructed = 0;
        Int32 Destroyed   = 0;
        Int32 Copies      = 0;
        Int32 Moves       = 0;
    };

    class FLifetimeValue final
    {
    public:

        explicit FLifetimeValue ( FLifetimeState &InState ) noexcept : State( &InState )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FLifetimeValue ( const FLifetimeValue &InOther ) noexcept : State( InOther.State )
        {
            ++State->Alive;
            ++State->Copies;
        }

        FLifetimeValue ( FLifetimeValue &&InOther ) noexcept : State( InOther.State )
        {
            ++State->Alive;
            ++State->Moves;
        }

        ~FLifetimeValue () noexcept
        {
            --State->Alive;
            ++State->Destroyed;
        }

    private:

        FLifetimeState *State;
    };

    static_assert( std::same_as<decltype( *std::declval<TOptional<Int32> &>() ), Int32 &> );
    static_assert( std::same_as<decltype( *std::declval<const TOptional<Int32> &>() ), const Int32 &> );
    static_assert( std::same_as<decltype( *std::declval<TOptional<Int32> &&>() ), Int32 &&> );

} // namespace

LUMEN_TEST( Optional_Constructor, EmptyDoesNotConstructValue )
{
    FLifetimeState State;

    {
        TOptional<FLifetimeValue> Optional;
        LUMEN_CHECK_FALSE( Optional.HasValue() );
        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Constructed, 0 );
    }

    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( Optional_Lifetime, EmplaceAndReset )
{
    FLifetimeState State;
    TOptional<FLifetimeValue> Optional;

    Optional.Emplace( State );
    LUMEN_CHECK_TRUE( Optional.HasValue() );
    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Constructed, 1 );

    Optional.Emplace( State );
    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Constructed, 2 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );

    Optional.Reset();
    Optional.Reset();
    LUMEN_CHECK_FALSE( Optional.HasValue() );
    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 2 );
}

LUMEN_TEST( Optional_Lifetime, CopyConstructsIndependentValue )
{
    FLifetimeState State;
    TOptional<FLifetimeValue> Source;
    Source.Emplace( State );

    {
        TOptional<FLifetimeValue> Copy( Source );
        LUMEN_CHECK_TRUE( Copy.HasValue() );
        LUMEN_CHECK_EQ( State.Alive, 2 );
        LUMEN_CHECK_EQ( State.Copies, 1 );
        LUMEN_CHECK_NOT_EQ( std::addressof( *Copy ), std::addressof( *Source ) );
    }

    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( Optional_Lifetime, MoveConstructsIndependentValue )
{
    FLifetimeState State;
    TOptional<FLifetimeValue> Source;
    Source.Emplace( State );

    {
        TOptional<FLifetimeValue> Moved( std::move( Source ) );
        LUMEN_CHECK_TRUE( Moved.HasValue() );
        LUMEN_CHECK_TRUE( Source.HasValue() );
        LUMEN_CHECK_EQ( State.Alive, 2 );
        LUMEN_CHECK_EQ( State.Moves, 1 );
    }

    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( Optional_Lifetime, EmptyCopyAndMoveDoNotConstructValue )
{
    FLifetimeState State;
    TOptional<FLifetimeValue> Source;
    TOptional<FLifetimeValue> Copy( Source );
    TOptional<FLifetimeValue> Moved( std::move( Source ) );

    LUMEN_CHECK_FALSE( Copy.HasValue() );
    LUMEN_CHECK_FALSE( Moved.HasValue() );
    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Constructed, 0 );
    LUMEN_CHECK_EQ( State.Copies, 0 );
    LUMEN_CHECK_EQ( State.Moves, 0 );
}

LUMEN_TEST( Optional_Monadic, Transform )
{
    TOptional<Int32> Optional( 21 );
    auto Result = Optional.Transform( [] ( const Int32 InValue )
    {
        return InValue * 2;
    } );

    LUMEN_CHECK_TRUE( Result.HasValue() );
    LUMEN_CHECK_EQ( *Result, 42 );
}

LUMEN_TEST( Optional_Monadic, EmptyTransformSkipsFunction )
{
    TOptional<Int32> Optional;
    Int32 Calls = 0;
    auto Result = Optional.Transform( [&Calls] ( const Int32 InValue )
    {
        ++Calls;
        return InValue;
    } );

    LUMEN_CHECK_FALSE( Result.HasValue() );
    LUMEN_CHECK_EQ( Calls, 0 );
}

LUMEN_TEST( Optional_Constructor, Constexpr )
{
    constexpr TOptional<Int32> Optional( 42 );
    constexpr auto Result = Optional.Transform( [] ( const Int32 InValue )
    {
        return InValue * 2;
    } );

    static_assert( Optional.HasValue() );
    static_assert( *Optional == 42 );
    static_assert( Result.HasValue() );
    static_assert( *Result == 84 );
}

} // namespace Lumen
