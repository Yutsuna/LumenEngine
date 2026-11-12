/**
 * @file ThreadTests.cpp
 * @brief Functional tests for FThread.
 */

#include <Memory/UniquePtr.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

#include <atomic>
#include <type_traits>
#include <utility>

namespace Lumen
{

static_assert( not std::is_copy_constructible_v<FThread> and not std::is_copy_assignable_v<FThread> );
static_assert( std::is_nothrow_move_constructible_v<FThread> and std::is_nothrow_move_assignable_v<FThread> );
static_assert( std::is_nothrow_default_constructible_v<FThread> );

LUMEN_TEST( Thread_State, DefaultIsEmpty )
{
    FThread Thread;

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    LUMEN_CHECK_FALSE( Thread.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Thread.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.GetStopToken().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
}

LUMEN_TEST( Thread_Execution, RunsOrdinaryCallableWithArguments )
{
    TAtomic<Int32> Result{ 0 };
    FThread Thread( [&Result] ( Int32 InValue )
    {
        Result.store( InValue, std::memory_order::release );
    }, 42 );

    LUMEN_CHECK_TRUE( Thread.IsJoinable() );
    LUMEN_CHECK_TRUE( Thread.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Thread.GetId() == FThread::GetCurrentId() );
    Thread.Join();

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    LUMEN_CHECK_EQ( Result.load( std::memory_order::acquire ), 42 );
}

LUMEN_TEST( Thread_Execution, SupportsMoveOnlyCallablesAndArguments )
{
    TAtomic<Int32> Result{ 0 };
    auto Function = [FunctionResult = MakeUnique<Int32>( 40 ), &Result] ( TUniquePtr<Int32> InValue ) mutable
    {
        Result.store( *FunctionResult + *InValue, std::memory_order::release );
    };
    FThread Thread( std::move( Function ), MakeUnique<Int32>( 2 ) );
    Thread.Join();

    LUMEN_CHECK_EQ( Result.load( std::memory_order::acquire ), 42 );
}

LUMEN_TEST( Thread_Cancellation, DestructorRequestsStopAndJoins )
{
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bFinished{ false };
    {
        FThread Thread( [&bStarted, &bFinished] ( FStopToken InStopToken )
        {
            bStarted.store( true, std::memory_order::release );
            while ( not InStopToken.StopRequested() )
            {
                FThread::Yield();
            }
            bFinished.store( true, std::memory_order::release );
        } );
        while ( not bStarted.load( std::memory_order::acquire ) )
        {
            FThread::Yield();
        }
    }

    LUMEN_CHECK_TRUE( bFinished.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Cancellation, ExplicitRequestStopIsShared )
{
    FThread Thread( [] ( FStopToken InStopToken )
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FStopToken Token = Thread.GetStopToken();

    LUMEN_CHECK_TRUE( Token.StopPossible() );
    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    LUMEN_CHECK_TRUE( Token.StopRequested() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
    Thread.Join();
}

LUMEN_TEST( Thread_Ownership, MoveTransfersThreadAndStopState )
{
    FThread Original( [] ( FStopToken InStopToken )
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FStopSource Source = Original.GetStopSource();
    FThread Moved            = std::move( Original );

    LUMEN_CHECK_FALSE( Original.IsJoinable() );
    LUMEN_CHECK_FALSE( Original.GetStopSource().StopPossible() );
    LUMEN_CHECK_TRUE( Moved.IsJoinable() );
    LUMEN_CHECK_TRUE( Source == Moved.GetStopSource() );
    LUMEN_CHECK_TRUE( Moved.RequestStop() );
    Moved.Join();
}

LUMEN_TEST( Thread_Ownership, SwapExchangesThreadAndStopState )
{
    FThread First( [] ( FStopToken InStopToken )
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Second;
    const FStopSource Source = First.GetStopSource();

    First.Swap( Second );
    LUMEN_CHECK_FALSE( First.IsJoinable() );
    LUMEN_CHECK_TRUE( Second.IsJoinable() );
    LUMEN_CHECK_TRUE( Source == Second.GetStopSource() );
    LUMEN_CHECK_TRUE( Second.RequestStop() );
    Second.Join();
}

LUMEN_TEST( Thread_Execution, DetachReleasesOwnership )
{
    TAtomic<Bool> bFinished{ false };
    FThread Thread( [&bFinished]
    {
        bFinished.store( true, std::memory_order::release );
    } );
    Thread.Detach();

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    while ( not bFinished.load( std::memory_order::acquire ) )
    {
        FThread::Yield();
    }
}

} // namespace Lumen
