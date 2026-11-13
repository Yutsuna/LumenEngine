/**
 * @file StopTokenTests.cpp
 * @brief Functional tests for FStopSource, FStopToken, and FStopCallback.
 */

#include "Thread/Thread.hpp"
#include <Memory/UniquePtr.hpp>
#include <Templates/StopToken.hpp>
#include <Tests/UnitTest.hpp>

#include <atomic>

#include <type_traits>
#include <utility>

namespace Lumen
{

static_assert( std::is_copy_constructible_v<FStopSource> and std::is_move_constructible_v<FStopSource> );
static_assert( std::is_copy_constructible_v<FStopToken> and std::is_move_constructible_v<FStopToken> );
static_assert( not std::is_copy_constructible_v<FStopCallback> and not std::is_move_constructible_v<FStopCallback> );
static_assert( std::is_nothrow_default_constructible_v<FStopSource> );
static_assert( std::is_nothrow_default_constructible_v<FStopToken> );
static_assert( noexcept( std::declval<FStopSource &>().RequestStop() ) );
static_assert( noexcept( std::declval<const FStopToken &>().StopRequested() ) );

LUMEN_TEST( StopToken_State, DefaultTokenHasNoState )
{
    const FStopToken Token;

    LUMEN_CHECK_FALSE( Token.StopPossible() );
    LUMEN_CHECK_FALSE( Token.StopRequested() );
}

LUMEN_TEST( StopToken_State, SourceAndTokenShareRequest )
{
    FStopSource Source;
    const FStopToken Token = Source.GetToken();

    LUMEN_CHECK_TRUE( Source.StopPossible() );
    LUMEN_CHECK_TRUE( Token.StopPossible() );
    LUMEN_CHECK_FALSE( Source.StopRequested() );
    LUMEN_CHECK_FALSE( Token.StopRequested() );

    LUMEN_CHECK_TRUE( Source.RequestStop() );
    LUMEN_CHECK_TRUE( Source.StopRequested() );
    LUMEN_CHECK_TRUE( Token.StopRequested() );
    LUMEN_CHECK_FALSE( Source.RequestStop() );
}

LUMEN_TEST( StopToken_Ownership, CopiesAndMovesRetainSharedState )
{
    FStopSource Source;
    const FStopSource CopiedSource   = Source;
    const FStopToken Token           = Source.GetToken();
    const FStopToken CopiedToken     = Token;
    const FStopToken &CopiedRefToken = CopiedToken;
    FStopSource MovedSource          = std::move( Source );

    LUMEN_CHECK_TRUE( CopiedSource == MovedSource );
    LUMEN_CHECK_TRUE( CopiedToken == Token );
    LUMEN_CHECK_TRUE( CopiedRefToken == Token );
    LUMEN_CHECK_FALSE( Source.StopPossible() );

    LUMEN_CHECK_TRUE( MovedSource.RequestStop() );
    LUMEN_CHECK_TRUE( CopiedSource.StopRequested() );
    LUMEN_CHECK_TRUE( CopiedToken.StopRequested() );
    LUMEN_CHECK_TRUE( CopiedRefToken.StopRequested() );
}

LUMEN_TEST( StopToken_Callback, RunsOnlyOnce )
{
    FStopSource Source;
    Int32 InvocationCount = 0;
    {
        FStopCallback Callback( Source.GetToken(), [&InvocationCount]
        {
            ++InvocationCount;
        } );
        LUMEN_CHECK_EQ( InvocationCount, 0 );
        LUMEN_CHECK_TRUE( Source.RequestStop() );
        LUMEN_CHECK_EQ( InvocationCount, 1 );
    }
    LUMEN_CHECK_FALSE( Source.RequestStop() );
    LUMEN_CHECK_EQ( InvocationCount, 1 );
}

LUMEN_TEST( StopToken_Callback, AlreadyStoppedRunsImmediately )
{
    FStopSource Source;
    LUMEN_CHECK_TRUE( Source.RequestStop() );

    Int32 InvocationCount = 0;
    FStopCallback Callback( Source.GetToken(), [&InvocationCount]
    {
        ++InvocationCount;
    } );
    LUMEN_CHECK_EQ( InvocationCount, 1 );
}

LUMEN_TEST( StopToken_Callback, DestructionUnregisters )
{
    FStopSource Source;
    Int32 InvocationCount = 0;
    {
        FStopCallback Callback( Source.GetToken(), [&InvocationCount]
        {
            ++InvocationCount;
        } );
    }

    LUMEN_CHECK_TRUE( Source.RequestStop() );
    LUMEN_CHECK_EQ( InvocationCount, 0 );
}

LUMEN_TEST( StopToken_Callback, CanDestroyItselfDuringExecution )
{
    FStopSource Source;
    TUniquePtr<FStopCallback> Callback;
    Callback = MakeUnique<FStopCallback>( Source.GetToken(), [&Callback]
    {
        Callback.Reset();
    } );

    LUMEN_CHECK_TRUE( Source.RequestStop() );
    LUMEN_CHECK_FALSE( Callback.IsValid() );
}

LUMEN_TEST( StopToken_Concurrency, ConcurrentRegistrationDoesNotLoseCallbacks )
{
    constexpr Int32 ThreadCount        = 4;
    constexpr Int32 CallbacksPerThread = 64;
    constexpr Int32 TotalCallbacks     = ThreadCount * CallbacksPerThread;

    FStopSource Source;
    const FStopToken Token = Source.GetToken();
    TAtomic<Int32> RegistrationCount{ 0 };
    TAtomic<Int32> InvocationCount{ 0 };
    FThread Workers[ThreadCount];

    for ( Int32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex )
    {
        Workers[ThreadIndex] = FThread( [Token, &RegistrationCount, &InvocationCount]
        {
            TUniquePtr<FStopCallback> Callbacks[CallbacksPerThread];
            for ( Int32 CallbackIndex = 0; CallbackIndex < CallbacksPerThread; ++CallbackIndex )
            {
                Callbacks[CallbackIndex] = MakeUnique<FStopCallback>( Token, [&InvocationCount]
                {
                    InvocationCount.fetch_add( 1, std::memory_order::relaxed );
                } );
                RegistrationCount.fetch_add( 1, std::memory_order::release );
            }

            while ( InvocationCount.load( std::memory_order::acquire ) != TotalCallbacks )
            {
                FThread::Yield();
            }
        } );
    }

    while ( RegistrationCount.load( std::memory_order::acquire ) != TotalCallbacks )
    {
        FThread::Yield();
    }
    LUMEN_CHECK_TRUE( Source.RequestStop() );

    for ( FThread &Worker : Workers )
    {
        Worker.Join();
    }
    LUMEN_CHECK_EQ( InvocationCount.load(), TotalCallbacks );
}

} // namespace Lumen
