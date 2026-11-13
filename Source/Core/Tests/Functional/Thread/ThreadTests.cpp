/**
 * @file ThreadTests.cpp
 * @brief Functional tests for FThread.
 */

#include <Memory/UniquePtr.hpp>
#include <Tests/UnitTest.hpp>
#include <Thread/Thread.hpp>

namespace Lumen
{

/**
 * @brief Compile-time contract checks for FThread.
 * @details FThread must stay move-only, nothrow-movable/default-constructible
 *          and nothrow-destructible. If any of these break, every downstream
 *          consumer breaks, so they are checked here rather than in user code.
 */
static_assert( not std::is_copy_constructible_v<FThread> and not std::is_copy_assignable_v<FThread> );
static_assert( std::is_nothrow_move_constructible_v<FThread> and std::is_nothrow_move_assignable_v<FThread> );
static_assert( std::is_nothrow_default_constructible_v<FThread> );
static_assert( std::is_nothrow_destructible_v<FThread> );

/** @brief Public operations are noexcept by contract. */
static_assert( noexcept( std::declval<FThread &>().Join() ) );
static_assert( noexcept( std::declval<FThread &>().Detach() ) );
static_assert( noexcept( std::declval<FThread &>().Swap( std::declval<FThread &>() ) ) );
static_assert( noexcept( std::declval<const FThread &>().IsJoinable() ) );
static_assert( noexcept( std::declval<FThread &>().RequestStop() ) );
static_assert( noexcept( std::declval<const FThread &>().GetStopSource() ) );
static_assert( noexcept( std::declval<const FThread &>().GetStopToken() ) );
static_assert( noexcept( std::declval<const FThread &>().GetId() ) );
static_assert( noexcept( std::declval<const FThread &>().GetNativeHandle() ) );
static_assert( noexcept( FThread::GetCurrentId() ) );
static_assert( noexcept( FThread::HardwareConcurrency() ) );
static_assert( noexcept( FThread::Yield() ) );

/**
 * Thread_State — construction, empty state and post-operation reset.
 */

LUMEN_TEST( Thread_State, DefaultIsEmpty )
{
    FThread Thread;

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    LUMEN_CHECK_FALSE( Thread.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Thread.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.GetStopToken().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
}

LUMEN_TEST( Thread_State, AfterJoinBecomesEmpty )
{
    FThread Thread( []
    {
        /* */
    } );
    LUMEN_CHECK_TRUE( Thread.IsJoinable() );
    Thread.Join();

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    LUMEN_CHECK_FALSE( Thread.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Thread.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.GetStopToken().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
}

LUMEN_TEST( Thread_State, AfterDetachBecomesEmpty )
{
    TAtomic<Bool> bFinished{ false };
    FThread Thread( [&bFinished]
    {
        bFinished.store( true, std::memory_order::release );
    } );
    Thread.Detach();

    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
    LUMEN_CHECK_FALSE( Thread.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Thread.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.GetStopToken().StopPossible() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );

    while ( not bFinished.load( std::memory_order::acquire ) )
    {
        FThread::Yield();
    }
}

LUMEN_TEST( Thread_State, MoveConstructedFromEmptyRemainsEmpty )
{
    FThread Source;
    FThread Moved( std::move( Source ) );

    LUMEN_CHECK_FALSE( Source.IsJoinable() );
    LUMEN_CHECK_FALSE( Source.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Moved.IsJoinable() );
    LUMEN_CHECK_FALSE( Moved.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( Moved.GetId().IsValid() );
}

LUMEN_TEST( Thread_State, MoveAssignedFromEmptyBecomesEmpty )
{
    FThread First( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Empty;

    First = std::move( Empty );

    LUMEN_CHECK_FALSE( First.IsJoinable() );
    LUMEN_CHECK_FALSE( First.GetStopSource().StopPossible() );
    LUMEN_CHECK_FALSE( First.GetId().IsValid() );
    LUMEN_CHECK_FALSE( Empty.IsJoinable() );
}

/**
 * Thread_Execution — callable shapes, arguments, decay and ownership.
 */

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

LUMEN_TEST( Thread_Execution, RunsVoidCallableWithoutArguments )
{
    TAtomic<Bool> bRan{ false };
    FThread Thread( [&bRan]
    {
        bRan.store( true, std::memory_order::release );
    } );
    Thread.Join();

    LUMEN_CHECK_TRUE( bRan.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Execution, RunsCallableWithMultipleArguments )
{
    TAtomic<Int32> Result{ 0 };
    FThread Thread( [&Result] ( Int32 InA, Int32 InB, Int32 InC )
    {
        Result.store( InA + InB + InC, std::memory_order::release );
    }, 10, 20, 12 );
    Thread.Join();

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

LUMEN_TEST( Thread_Execution, SupportsMovedCopyableCallable )
{
    TAtomic<Int32> Result{ 0 };
    auto Callable = [&Result] ( Int32 InValue )
    {
        Result.store( InValue, std::memory_order::release );
    };
    FThread Thread( std::move( Callable ), 42 );
    Thread.Join();

    LUMEN_CHECK_EQ( Result.load( std::memory_order::acquire ), 42 );
}

LUMEN_TEST( Thread_Execution, IgnoresCallableReturnValue )
{
    TAtomic<Bool> bRan{ false };
    FThread Thread( [&bRan] () -> Int32
    {
        bRan.store( true, std::memory_order::release );
        return 42;
    } );
    Thread.Join();

    LUMEN_CHECK_TRUE( bRan.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Execution, CallableTakingStopTokenReceivesIt )
{
    TAtomic<Bool> bReceivedStopToken{ false };
    TAtomic<Int32> Result{ 0 };
    FThread Thread( [&] ( const FStopToken &InStopToken, Int32 InValue ) noexcept -> void
    {
        bReceivedStopToken.store( not InStopToken.StopRequested(), std::memory_order::release );
        Result.store( InValue, std::memory_order::release );
    }, 42 );
    Thread.Join();

    LUMEN_CHECK_TRUE( bReceivedStopToken.load( std::memory_order::acquire ) );
    LUMEN_CHECK_EQ( Result.load( std::memory_order::acquire ), 42 );
}

LUMEN_TEST( Thread_Execution, CallableWithStopTokenAndMoveOnlyArgument )
{
    TAtomic<Int32> Result{ 0 };
    FThread Thread( [&Result] ( const FStopToken & /*InStopToken*/, TUniquePtr<Int32> InValue ) noexcept -> void
    {
        Result.store( *InValue, std::memory_order::release );
    }, MakeUnique<Int32>( 42 ) );
    Thread.Join();

    LUMEN_CHECK_EQ( Result.load( std::memory_order::acquire ), 42 );
}

LUMEN_TEST( Thread_Execution, CallableMayIgnoreStopToken )
{
    TAtomic<Bool> bReceived{ false };
    FThread Thread( [&bReceived] ( const FStopToken & /*InStopToken*/ ) noexcept -> void
    {
        bReceived.store( true, std::memory_order::release );
    } );
    Thread.Join();

    LUMEN_CHECK_TRUE( bReceived.load( std::memory_order::acquire ) );
}

/**
 * Thread_Cancellation — stop state ownership and propagation.
 */

LUMEN_TEST( Thread_Cancellation, DestructorRequestsStopAndJoins )
{
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bFinished{ false };
    {
        FThread Thread( [&bStarted, &bFinished] ( const FStopToken &InStopToken ) noexcept -> void
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
    FThread Thread( [] ( const FStopToken &InStopToken ) noexcept -> void
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

LUMEN_TEST( Thread_Cancellation, RequestStopReturnsTrueOnlyOnce )
{
    FThread Thread( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
    LUMEN_CHECK_FALSE( Thread.RequestStop() );
    Thread.Join();
}

LUMEN_TEST( Thread_Cancellation, StopTokenExposedEvenWithoutStopTokenParameter )
{
    TAtomic<Bool> bRan{ false };
    FThread Thread( [&bRan]
    {
        bRan.store( true, std::memory_order::release );
    } );
    const FStopToken Token = Thread.GetStopToken();

    LUMEN_CHECK_TRUE( Token.StopPossible() );
    LUMEN_CHECK_FALSE( Token.StopRequested() );
    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    LUMEN_CHECK_TRUE( Token.StopRequested() );

    Thread.Join();
    LUMEN_CHECK_TRUE( bRan.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Cancellation, RequestStopBeforeThreadStartsIsObserved )
{
    TAtomic<Bool> bObserved{ false };
    FThread Thread( [&bObserved] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
        bObserved.store( true, std::memory_order::release );
    } );
    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    Thread.Join();

    LUMEN_CHECK_TRUE( bObserved.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Cancellation, MultipleGetStopSourceCallsReturnEqualSources )
{
    FThread Thread( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );

    LUMEN_CHECK_TRUE( Thread.GetStopSource() == Thread.GetStopSource() );
    LUMEN_CHECK_TRUE( Thread.GetStopSource() == Thread.GetStopSource() );

    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    Thread.Join();
}

LUMEN_TEST( Thread_Cancellation, TokenRemainsValidAfterJoin )
{
    FThread Thread( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FStopToken Token = Thread.GetStopToken();
    LUMEN_CHECK_TRUE( Thread.RequestStop() );
    Thread.Join();

    LUMEN_CHECK_TRUE( Token.StopRequested() );
}

/**
 * Thread_Ownership — move, swap, detach.
 */

LUMEN_TEST( Thread_Ownership, MoveTransfersThreadAndStopState )
{
    FThread Original( [] ( const FStopToken &InStopToken ) noexcept -> void
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

LUMEN_TEST( Thread_Ownership, MoveAssignmentTransfersThreadAndStopState )
{
    FThread First( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Second( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FStopSource Source = Second.GetStopSource();

    First = std::move( Second );

    LUMEN_CHECK_FALSE( Second.IsJoinable() );
    LUMEN_CHECK_FALSE( Second.GetStopSource().StopPossible() );
    LUMEN_CHECK_TRUE( First.IsJoinable() );
    LUMEN_CHECK_TRUE( Source == First.GetStopSource() );
    LUMEN_CHECK_TRUE( First.RequestStop() );
    First.Join();
}

LUMEN_TEST( Thread_Ownership, MoveAssignmentRequestsStopAndJoinsCurrentThread )
{
    TAtomic<Bool> bFirstFinished{ false };
    FThread First( [&bFirstFinished] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
        bFirstFinished.store( true, std::memory_order::release );
    } );
    FThread Second( []
    {
        /* */
    } );

    First = std::move( Second );

    LUMEN_CHECK_TRUE( bFirstFinished.load( std::memory_order::acquire ) );
    LUMEN_CHECK_TRUE( First.IsJoinable() );
    First.Join();
}

LUMEN_TEST( Thread_Ownership, MovedFromThreadCannotRequestStop )
{
    FThread Original( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Moved( std::move( Original ) );

    LUMEN_CHECK_FALSE( Original.RequestStop() );
    LUMEN_CHECK_FALSE( Original.GetStopSource().StopPossible() );
    LUMEN_CHECK_TRUE( Moved.RequestStop() );
    Moved.Join();
}

LUMEN_TEST( Thread_Ownership, SwapExchangesThreadAndStopState )
{
    FThread First( [] ( const FStopToken &InStopToken ) noexcept -> void
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

LUMEN_TEST( Thread_Ownership, SwapBetweenTwoJoinableThreads )
{
    FThread First( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Second( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FStopSource FirstSource  = First.GetStopSource();
    const FStopSource SecondSource = Second.GetStopSource();

    First.Swap( Second );

    LUMEN_CHECK_TRUE( First.IsJoinable() );
    LUMEN_CHECK_TRUE( Second.IsJoinable() );
    LUMEN_CHECK_TRUE( First.GetStopSource() == SecondSource );
    LUMEN_CHECK_TRUE( Second.GetStopSource() == FirstSource );

    LUMEN_CHECK_TRUE( First.RequestStop() );
    LUMEN_CHECK_TRUE( Second.RequestStop() );
    First.Join();
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

LUMEN_TEST( Thread_Ownership, DetachedThreadNotJoinedByDestructor )
{
    TAtomic<Bool> bStarted{ false };
    TAtomic<Bool> bAllowFinish{ false };
    TAtomic<Bool> bFinished{ false };

    {
        FThread Thread( [&]
        {
            bStarted.store( true, std::memory_order::release );
            while ( not bAllowFinish.load( std::memory_order::acquire ) )
            {
                FThread::Yield();
            }
            bFinished.store( true, std::memory_order::release );
        } );
        while ( not bStarted.load( std::memory_order::acquire ) )
        {
            FThread::Yield();
        }
        Thread.Detach();
    }

    bAllowFinish.store( true, std::memory_order::release );

    while ( not bFinished.load( std::memory_order::acquire ) )
    {
        FThread::Yield();
    }
    LUMEN_CHECK_TRUE( bFinished.load( std::memory_order::acquire ) );
}

/**
 * Thread_Lifetime — destructor edge cases.
 */

LUMEN_TEST( Thread_Lifetime, DestructorOfFinishedThreadDoesNotHang )
{
    TAtomic<Bool> bFinished{ false };
    {
        FThread Thread( [&bFinished]
        {
            bFinished.store( true, std::memory_order::release );
        } );
        while ( not bFinished.load( std::memory_order::acquire ) )
        {
            FThread::Yield();
        }
    }
}

LUMEN_TEST( Thread_Lifetime, DestructorOfJoinedThreadIsNoOp )
{
    FThread Thread( []
    {
        /* */
    } );
    Thread.Join();
}

LUMEN_TEST( Thread_Lifetime, DestructorOfEmptyThreadIsNoOp )
{
    FThread Thread;
}

/**
 * Thread_Platform — id, hardware concurrency, yield, native handle.
 */

LUMEN_TEST( Thread_Platform, GetCurrentIdIsValidFromAnyThread )
{
    LUMEN_CHECK_TRUE( FThread::GetCurrentId().IsValid() );
}

LUMEN_TEST( Thread_Platform, GetCurrentIdMatchesInsideThread )
{
    FThread *ThreadPtr = nullptr;
    TAtomic<Bool> bThreadAvailable{ false };
    TAtomic<Bool> bMatched{ false };

    FThread Thread( [&]
    {
        while ( not bThreadAvailable.load( std::memory_order::acquire ) )
        {
            FThread::Yield();
        }
        bMatched.store( FThread::GetCurrentId() == ThreadPtr->GetId(), std::memory_order::release );
    } );

    ThreadPtr = &Thread;
    bThreadAvailable.store( true, std::memory_order::release );
    Thread.Join();

    LUMEN_CHECK_TRUE( bMatched.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Platform, GetCurrentIdInsideThreadDiffersFromOutside )
{
    const FThread::FId MainId = FThread::GetCurrentId();
    TAtomic<Bool> bDifferent{ false };

    FThread Thread( [&]
    {
        bDifferent.store( not( FThread::GetCurrentId() == MainId ), std::memory_order::release );
    } );
    Thread.Join();

    LUMEN_CHECK_TRUE( bDifferent.load( std::memory_order::acquire ) );
}

LUMEN_TEST( Thread_Platform, DistinctThreadsHaveDistinctIds )
{
    FThread First( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    FThread Second( [] ( const FStopToken &InStopToken ) noexcept -> void
    {
        while ( not InStopToken.StopRequested() )
        {
            FThread::Yield();
        }
    } );
    const FThread::FId FirstId  = First.GetId();
    const FThread::FId SecondId = Second.GetId();

    LUMEN_CHECK_FALSE( FirstId == SecondId );

    LUMEN_CHECK_TRUE( First.RequestStop() );
    LUMEN_CHECK_TRUE( Second.RequestStop() );
    First.Join();
    Second.Join();
}

LUMEN_TEST( Thread_Platform, HardwareConcurrencyReturnsAtLeastOne )
{
    LUMEN_CHECK_TRUE( FThread::HardwareConcurrency() >= 1U );
}

LUMEN_TEST( Thread_Platform, YieldIsCallableAndRepeatable )
{
    FThread::Yield();
    FThread::Yield();
}

LUMEN_TEST( Thread_Platform, GetNativeHandleOfJoinableThreadIsCallable )
{
    FThread Thread( []
    {
        /* */
    } );

    LUMEN_CHECK_TRUE( Thread.GetNativeHandle() != 0 );
    LUMEN_CHECK_TRUE( Thread.IsJoinable() );
    Thread.Join();
}

LUMEN_TEST( Thread_Platform, GetNativeHandleOfEmptyThreadIsCallable )
{
    FThread Thread;
    static_cast<void>( Thread.GetNativeHandle() );
    LUMEN_CHECK_FALSE( Thread.IsJoinable() );
}

} // namespace Lumen
