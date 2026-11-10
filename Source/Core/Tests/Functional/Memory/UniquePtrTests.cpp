/**
 * @file UniquePtrTests.cpp
 * @brief Functional unit tests for TUniquePtr (single object and array), custom deleters, MakeUnique and the monadic mixin.
 */

#include <Memory/UniquePtr.hpp>
#include <Templates/Monadic.hpp>
#include <Tests/UnitTest.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    /** @brief Lifetime bookkeeping shared by every test of this file. */
    struct FUniqueState
    {
        Int32 Alive       = 0;
        Int32 Constructed = 0;
        Int32 Destroyed   = 0;
    };

    /** @brief Value object whose lifetime is traced into an FUniqueState. */
    class FUniqueLifetimeObject final
    {
    public:

        explicit FUniqueLifetimeObject ( FUniqueState &InState ) noexcept : State( &InState ), Value( 0 )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FUniqueLifetimeObject ( FUniqueState &InState, const Int32 InValue ) noexcept : State( &InState ), Value( InValue )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FUniqueLifetimeObject ( const FUniqueLifetimeObject &InOther ) noexcept : State( InOther.State ), Value( InOther.Value )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        ~FUniqueLifetimeObject () noexcept
        {
            --State->Alive;
            ++State->Destroyed;
        }

        [[nodiscard]] Int32 GetValue () const noexcept
        {
            return Value;
        }

        void SetValue ( const Int32 InValue ) noexcept
        {
            Value = InValue;
        }

    private:

        FUniqueState *State;
        Int32 Value;
    };

    /** @brief Polymorphic base with a virtual destructor, observed through owning base pointers. */
    struct FUniqueBase
    {
        virtual ~FUniqueBase () noexcept = default;

        [[nodiscard]] virtual Int32 Id () const noexcept
        {
            return 1;
        }
    };

    /** @brief Concrete leaf whose destruction is traced into an FUniqueState. */
    struct FUniqueDerived final : FUniqueBase
    {
        explicit FUniqueDerived ( FUniqueState &InState ) noexcept : State( &InState ), Extra( 0 )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FUniqueDerived ( FUniqueState &InState, const Int32 InExtra ) noexcept : State( &InState ), Extra( InExtra )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        ~FUniqueDerived () noexcept override
        {
            --State->Alive;
            ++State->Destroyed;
        }

        [[nodiscard]] Int32 Id () const noexcept override
        {
            return 2;
        }

        FUniqueState *State;
        Int32 Extra;
    };

    /** @brief Counts every deletion performed by a custom deleter. */
    struct FDeletionCounter
    {
        Int32 *Count;

        void operator()( const FUniqueLifetimeObject *Ptr ) const noexcept
        {
            ++( *Count );
            delete Ptr;
        }
    };

    /** @brief Remembers the exact pointer it was asked to delete. */
    struct FPointerCapturingDeleter
    {
        const FUniqueLifetimeObject **CapturedSite;

        void operator()( const FUniqueLifetimeObject *Ptr ) const noexcept
        {
            *CapturedSite = Ptr;

            delete Ptr;
        }
    };

    /** @brief Optionally-traced array element; default construction contributes nothing. */

    /** @brief Array element tracing its lifetime; default construction contributes nothing. */
    struct FArrayElement
    {
        FArrayElement () noexcept : State( nullptr ), Value( 0 )
        {
        }

        FArrayElement ( const FArrayElement &InOther ) noexcept : State( InOther.State ), Value( InOther.Value )
        {
            if ( State != nullptr )
            {
                ++State->Alive;
                ++State->Constructed;
            }
        }

        void Bind ( FUniqueState &InState ) noexcept
        {
            State = &InState;
            Value = 1;

            ++State->Alive;
            ++State->Constructed;
        }

        ~FArrayElement () noexcept
        {
            if ( State != nullptr )
            {
                --State->Alive;
                ++State->Destroyed;
            }
        }

        FUniqueState *State;
        Int32 Value;
    };

    static_assert( not std::is_copy_constructible_v<TUniquePtr<FUniqueLifetimeObject>> );
    static_assert( not std::is_copy_assignable_v<TUniquePtr<FUniqueLifetimeObject>> );
    static_assert( std::is_move_constructible_v<TUniquePtr<FUniqueLifetimeObject>> );
    static_assert( std::is_move_assignable_v<TUniquePtr<FUniqueLifetimeObject>> );
    static_assert( std::is_constructible_v<TUniquePtr<FUniqueBase>, TUniquePtr<FUniqueDerived> &&> );
    static_assert( not std::is_constructible_v<TUniquePtr<FUniqueDerived>, TUniquePtr<FUniqueBase> &&> );
    /**
     * concept probes for the array specialization — the primary template owns these members,
     * the Type[] specialization must not re-expose them.
     */
    struct FMonadicIdentity
    {
        Int32 operator()( const Int32 Value ) const noexcept
        {
            return Value;
        }
    };

    template <typename PooledType>
    concept CHasMonadicEmplace = requires { PooledType::MonadicEmplace( FMonadicIdentity{}, Int32{} ); };

    template <typename PooledType, typename ReboundType>
    concept CHasRebind = requires { typename PooledType::template Rebind<ReboundType>; };

    static_assert( not CHasMonadicEmplace<TUniquePtr<Int32[]>> );
    static_assert( not CHasRebind<TUniquePtr<Int32[]>, Int32> );
    static_assert( CHasMonadicEmplace<TUniquePtr<Int32>> );
    static_assert( CHasRebind<TUniquePtr<Int32>, Int64> );

} // namespace

/**
 * TUniquePtr — Construction
 */

LUMEN_TEST( UniquePtr_Object_Constructor, DefaultConstructsEmpty )
{
    TUniquePtr<FUniqueLifetimeObject> Empty;

    LUMEN_CHECK_FALSE( Empty.IsValid() );
    LUMEN_CHECK_TRUE( Empty.Get() == nullptr );
    LUMEN_CHECK_FALSE( static_cast<Bool>( Empty ) );

    LUMEN_CHECK_TRUE( Empty.Release() == nullptr );
}

LUMEN_TEST( UniquePtr_Object_Constructor, NullLiteralConstructsEmpty )
{
    TUniquePtr<FUniqueLifetimeObject> Null( nullptr );

    LUMEN_CHECK_FALSE( Null.IsValid() );
    LUMEN_CHECK_TRUE( Null.Get() == nullptr );

    Null.Reset( nullptr );
    Null = nullptr;

    LUMEN_CHECK_FALSE( Null.IsValid() );
}

LUMEN_TEST( UniquePtr_Object_Constructor, RawPointerTakesOwnership )
{
    FUniqueState State;

    {
        TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State, 7 ) );

        LUMEN_CHECK_TRUE( Ptr.IsValid() );
        LUMEN_CHECK_TRUE( static_cast<Bool>( Ptr ) );
        LUMEN_CHECK_EQ( Ptr->GetValue(), 7 );
        LUMEN_CHECK_EQ( State.Alive, 1 );
    }

    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( UniquePtr_Object_Constructor, MakeUniqueForwardsArguments )
{
    FUniqueState State;

    {
        auto Ptr = MakeUnique<FUniqueLifetimeObject>( State, 41 );

        LUMEN_CHECK_TRUE( Ptr.IsValid() );
        LUMEN_CHECK_EQ( Ptr->GetValue(), 41 );
        LUMEN_CHECK_EQ( State.Constructed, 1 );
    }

    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( UniquePtr_Object_Constructor, CustomDeleterRunsOnDestruction )
{
    FUniqueState State;
    Int32 Deletions = 0;

    {
        TUniquePtr<FUniqueLifetimeObject, FDeletionCounter> Ptr( new FUniqueLifetimeObject( State ),
                                                                 FDeletionCounter{ &Deletions } );

        LUMEN_CHECK_TRUE( Ptr.IsValid() );
        LUMEN_CHECK_EQ( Deletions, 0 );
    }

    LUMEN_CHECK_EQ( Deletions, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

/**
 * TUniquePtr — Move
 */

LUMEN_TEST( UniquePtr_Object_Move, ConstructorTransfersOwnership )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Origin( new FUniqueLifetimeObject( State, 3 ) );

    TUniquePtr<FUniqueLifetimeObject> Target( std::move( Origin ) );

    LUMEN_CHECK_TRUE( Target.IsValid() );
    LUMEN_CHECK_EQ( Target->GetValue(), 3 );
    LUMEN_CHECK_FALSE( Origin.IsValid() );
    LUMEN_CHECK_TRUE( Origin.Get() == nullptr );

    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Object_Move, AssignmentTransfersOwnership )
{
    FUniqueState FirstState;
    FUniqueState SecondState;
    TUniquePtr<FUniqueLifetimeObject> First( new FUniqueLifetimeObject( FirstState, 1 ) );
    TUniquePtr<FUniqueLifetimeObject> Second( new FUniqueLifetimeObject( SecondState, 2 ) );

    First = std::move( Second );

    LUMEN_CHECK_TRUE( First.IsValid() );
    LUMEN_CHECK_EQ( First->GetValue(), 2 );
    LUMEN_CHECK_FALSE( Second.IsValid() );
    LUMEN_CHECK_TRUE( Second.Get() == nullptr );

    LUMEN_CHECK_EQ( FirstState.Destroyed, 1 );
    LUMEN_CHECK_EQ( FirstState.Alive, 0 );
    LUMEN_CHECK_EQ( SecondState.Alive, 1 );
}

LUMEN_TEST( UniquePtr_Object_Move, SelfAssignmentIsHarmless )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State ) );

    TUniquePtr<FUniqueLifetimeObject> &SelfSource = Ptr;
    Ptr                                           = std::move( SelfSource );

    LUMEN_CHECK_TRUE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Object_Move, NullAssignmentDestroysObject )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State ) );

    Ptr = nullptr;

    LUMEN_CHECK_FALSE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( UniquePtr_Object_Converting, UpcastsToBaseOnMoveConstruction )
{
    FUniqueState State;
    TUniquePtr<FUniqueDerived> Derived( new FUniqueDerived( State, 9 ) );

    TUniquePtr<FUniqueBase> Base( std::move( Derived ) );

    LUMEN_CHECK_TRUE( Base.IsValid() );
    LUMEN_CHECK_EQ( Base->Id(), 2 );
    LUMEN_CHECK_FALSE( Derived.IsValid() );
    LUMEN_CHECK_EQ( State.Alive, 1 );
}

LUMEN_TEST( UniquePtr_Object_Converting, UpcastsToBaseOnMoveAssignment )
{
    FUniqueState FirstState;
    FUniqueState SecondState;
    TUniquePtr<FUniqueDerived> First( new FUniqueDerived( FirstState, 1 ) );
    TUniquePtr<FUniqueBase> Base( std::move( First ) );

    Base = MakeUnique<FUniqueDerived>( SecondState, 4 );

    LUMEN_CHECK_TRUE( Base.IsValid() );
    LUMEN_CHECK_EQ( Base->Id(), 2 );

    LUMEN_CHECK_EQ( FirstState.Destroyed, 1 );
    LUMEN_CHECK_EQ( FirstState.Alive, 0 );
    LUMEN_CHECK_EQ( SecondState.Alive, 1 );
}

LUMEN_TEST( UniquePtr_Object_Converting, DerivedObjectDestroyedThroughBasePointer )
{
    FUniqueState State;

    {
        TUniquePtr<FUniqueDerived> Derived( new FUniqueDerived( State ) );
        TUniquePtr<FUniqueBase> Base( std::move( Derived ) );

        LUMEN_CHECK_EQ( State.Alive, 1 );
    }

    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

/**
 * TUniquePtr — Access
 */

LUMEN_TEST( UniquePtr_Object_Access, DereferenceAndMutate )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State, 2 ) );

    Ptr->SetValue( 20 );
    LUMEN_CHECK_EQ( Ptr->GetValue(), 20 );

    ( *Ptr ).SetValue( 32 );
    LUMEN_CHECK_EQ( ( *Ptr ).GetValue(), 32 );
    LUMEN_CHECK_EQ( Ptr.Get()->GetValue(), 32 );
}

/**
 * TUniquePtr — Ownership
 */

LUMEN_TEST( UniquePtr_Object_Ownership, ReleaseRelinquishesOwnership )
{
    FUniqueState State;
    Int32 Deletions = 0;
    TUniquePtr<FUniqueLifetimeObject, FDeletionCounter> Ptr( new FUniqueLifetimeObject( State, 11 ),
                                                             FDeletionCounter{ &Deletions } );

    FUniqueLifetimeObject *Raw = Ptr.Release();

    LUMEN_CHECK_FALSE( Ptr.IsValid() );
    LUMEN_CHECK_TRUE( Raw != nullptr );
    LUMEN_CHECK_EQ( Raw->GetValue(), 11 );
    LUMEN_CHECK_EQ( Deletions, 0 );
    LUMEN_CHECK_EQ( State.Alive, 1 );

    delete Raw;

    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( Deletions, 0 );
}

LUMEN_TEST( UniquePtr_Object_Ownership, ResetReplacesManagedObject )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State, 1 ) );

    Ptr.Reset( new FUniqueLifetimeObject( State, 2 ) );

    LUMEN_CHECK_TRUE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( Ptr->GetValue(), 2 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
    LUMEN_CHECK_EQ( State.Alive, 1 );

    Ptr.Reset();

    LUMEN_CHECK_FALSE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( State.Destroyed, 2 );
    LUMEN_CHECK_EQ( State.Alive, 0 );

    Ptr.Reset( new FUniqueLifetimeObject( State, 3 ) );
    Ptr = nullptr;

    LUMEN_CHECK_EQ( State.Destroyed, 3 );
}

LUMEN_TEST( UniquePtr_Object_Ownership, CustomDeleterReceivesTheExactPointer )
{
    FUniqueState State;
    const FUniqueLifetimeObject *Captured = nullptr;
    TUniquePtr<FUniqueLifetimeObject, FPointerCapturingDeleter> Ptr( new FUniqueLifetimeObject( State ),
                                                                     FPointerCapturingDeleter{ &Captured } );

    const FUniqueLifetimeObject *Raw = Ptr.Get();
    Ptr                              = nullptr;

    LUMEN_CHECK_EQ( Captured, Raw );
    LUMEN_CHECK_FALSE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( UniquePtr_Object_Ownership, ResetWithCustomDeleterDeletesEachOldObject )
{
    FUniqueState State;
    Int32 Deletions = 0;
    TUniquePtr<FUniqueLifetimeObject, FDeletionCounter> Ptr( new FUniqueLifetimeObject( State ), FDeletionCounter{ &Deletions } );

    Ptr.Reset( new FUniqueLifetimeObject( State ) );
    Ptr.Reset( new FUniqueLifetimeObject( State ) );

    LUMEN_CHECK_EQ( Deletions, 2 );
    LUMEN_CHECK_EQ( State.Destroyed, 2 );
    LUMEN_CHECK_EQ( State.Alive, 1 );

    Ptr = nullptr;

    LUMEN_CHECK_EQ( Deletions, 3 );
    LUMEN_CHECK_EQ( State.Destroyed, 3 );
}

/**
 * TUniquePtr — Deleter Access
 */

LUMEN_TEST( UniquePtr_Object_Deleter, GetDeleterGivesMutableAndConstAccess )
{
    FUniqueState State;
    Int32 Deletions      = 0;
    Int32 OtherDeletions = 0;

    {
        TUniquePtr<FUniqueLifetimeObject, FDeletionCounter> Ptr( new FUniqueLifetimeObject( State ),
                                                                 FDeletionCounter{ &Deletions } );

        const TUniquePtr<FUniqueLifetimeObject, FDeletionCounter> &ConstPtr = Ptr;

        LUMEN_CHECK_TRUE( ConstPtr.GetDeleter().Count == &Deletions );

        Ptr.GetDeleter().Count = &OtherDeletions;

        LUMEN_CHECK_TRUE( Ptr.GetDeleter().Count == &OtherDeletions );
        LUMEN_CHECK_TRUE( ConstPtr.GetDeleter().Count == &OtherDeletions );
    }

    LUMEN_CHECK_EQ( Deletions, 0 );
    LUMEN_CHECK_EQ( OtherDeletions, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

/**
 * TUniquePtr — Monadic Operations (via FMonadic)
 */

LUMEN_TEST( UniquePtr_Monadic_Transform, BuildsANewOwnedPointer )
{
    FUniqueState State;
    TUniquePtr<FUniqueDerived> Ptr( new FUniqueDerived( State, 21 ) );

    TUniquePtr<FUniqueDerived> Result = std::move( Ptr ).Transform( [&State] ( const FUniqueDerived &InObject )
    {
        return FUniqueDerived( State, InObject.Extra * 2 );
    } );

    LUMEN_CHECK_TRUE( Result.IsValid() );
    LUMEN_CHECK_EQ( Result->Extra, 42 );
    LUMEN_CHECK_TRUE( Ptr.Get() == nullptr );
    LUMEN_CHECK_EQ( State.Alive, 1 );
    LUMEN_CHECK_EQ( State.Destroyed, 1 );
}

LUMEN_TEST( UniquePtr_Monadic_Transform, LValueSourceStaysValid )
{
    FUniqueState State;
    TUniquePtr<FUniqueDerived> Ptr( new FUniqueDerived( State, 21 ) );

    TUniquePtr<FUniqueDerived> Result = Ptr.Transform( [&State] ( const FUniqueDerived &InObject )
    {
        return FUniqueDerived( State, InObject.Extra + 1 );
    } );

    LUMEN_CHECK_TRUE( Result.IsValid() );
    LUMEN_CHECK_EQ( Result->Extra, 22 );
    LUMEN_CHECK_TRUE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( Ptr->Extra, 21 );
    LUMEN_CHECK_EQ( State.Alive, 2 );
    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Monadic_Transform, OnNullSkipsFunctionAndStaysEmpty )
{
    Int32 Calls = 0;
    FUniqueState DummyState;
    TUniquePtr<FUniqueDerived> Empty;

    TUniquePtr<FUniqueDerived> Result = std::move( Empty ).Transform( [&Calls, &DummyState] ( const FUniqueDerived & )
    {
        ++Calls;

        return FUniqueDerived( DummyState, 0 );
    } );

    LUMEN_CHECK_FALSE( Result.IsValid() );
    LUMEN_CHECK_EQ( Calls, 0 );
    LUMEN_CHECK_FALSE( Empty.IsValid() );
    LUMEN_CHECK_EQ( DummyState.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Monadic_AndThen, ChainsToTheReturnedPointer )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State, 8 ) );

    TUniquePtr<FUniqueBase> Result = Ptr.AndThen( [&State] ( const FUniqueLifetimeObject &InObject )
    {
        return TUniquePtr<FUniqueBase>( new FUniqueDerived( State, InObject.GetValue() ) );
    } );

    LUMEN_CHECK_TRUE( Result.IsValid() );
    LUMEN_CHECK_EQ( Result->Id(), 2 );
    LUMEN_CHECK_TRUE( Ptr.IsValid() );
}

LUMEN_TEST( UniquePtr_Monadic_AndThen, OnNullSkipsFunction )
{
    Int32 Calls = 0;
    TUniquePtr<FUniqueLifetimeObject> Empty;

    TUniquePtr<Int32> Result = std::move( Empty ).AndThen( [&Calls] ( const FUniqueLifetimeObject & )
    {
        ++Calls;

        return TUniquePtr<Int32>();
    } );

    LUMEN_CHECK_FALSE( Result.IsValid() );
    LUMEN_CHECK_EQ( Calls, 0 );
    LUMEN_CHECK_FALSE( Empty.IsValid() );
}

LUMEN_TEST( UniquePtr_Monadic_OrElse, ValidPointerIsReturnedUnchanged )
{
    FUniqueState State;
    Int32 Calls = 0;
    TUniquePtr<FUniqueLifetimeObject> Ptr( new FUniqueLifetimeObject( State, 14 ) );

    TUniquePtr<FUniqueLifetimeObject> Result = std::move( Ptr ).OrElse( [&Calls, &State]
    {
        ++Calls;

        return MakeUnique<FUniqueLifetimeObject>( State, 0 );
    } );

    LUMEN_CHECK_TRUE( Result.IsValid() );
    LUMEN_CHECK_EQ( Result->GetValue(), 14 );
    LUMEN_CHECK_EQ( Calls, 0 );
    LUMEN_CHECK_FALSE( Ptr.IsValid() );
    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Monadic_OrElse, RecoveredPointerOnNull )
{
    FUniqueState State;
    Int32 Calls = 0;
    TUniquePtr<FUniqueLifetimeObject> Empty;

    TUniquePtr<FUniqueLifetimeObject> Result = std::move( Empty ).OrElse( [&Calls, &State]
    {
        ++Calls;

        return MakeUnique<FUniqueLifetimeObject>( State, 99 );
    } );

    LUMEN_CHECK_TRUE( Result.IsValid() );
    LUMEN_CHECK_EQ( Result->GetValue(), 99 );
    LUMEN_CHECK_EQ( Calls, 1 );
    LUMEN_CHECK_FALSE( Empty.IsValid() );
}

LUMEN_TEST( UniquePtr_Monadic_Inspect, OnlyInvokesOnValidPointer )
{
    FUniqueState State;
    Int32 Calls = 0;
    TUniquePtr<FUniqueLifetimeObject> Valid( new FUniqueLifetimeObject( State, 1 ) );
    TUniquePtr<FUniqueLifetimeObject> Empty;

    Valid.Inspect( [&Calls] ( const FUniqueLifetimeObject & ) noexcept
    {
        ++Calls;
    } );
    Empty.Inspect( [&Calls] ( const FUniqueLifetimeObject & ) noexcept
    {
        ++Calls;
    } );

    LUMEN_CHECK_EQ( Calls, 1 );
    LUMEN_CHECK_TRUE( Valid.IsValid() );
    LUMEN_CHECK_FALSE( Empty.IsValid() );
}

LUMEN_TEST( UniquePtr_Monadic_ValueOr, ReturnsOwnedValueOrFallback )
{
    FUniqueState State;
    TUniquePtr<FUniqueLifetimeObject> Valid( new FUniqueLifetimeObject( State, 42 ) );
    TUniquePtr<FUniqueLifetimeObject> Empty;

    FUniqueLifetimeObject OwnedResult = Valid.ValueOr( FUniqueLifetimeObject( State, 7 ) );

    LUMEN_CHECK_EQ( OwnedResult.GetValue(), 42 );

    FUniqueLifetimeObject FallbackResult = Empty.ValueOr( FUniqueLifetimeObject( State, 7 ) );

    LUMEN_CHECK_EQ( FallbackResult.GetValue(), 7 );
}

LUMEN_TEST( UniquePtr_Monadic_Emplace, BuildsThePointerThroughTheStaticHook )
{
    TUniquePtr<Int32> Emplaced = TUniquePtr<Int32>::MonadicEmplace( [] ( const Int32 InValue ) noexcept
    {
        return InValue * 21;
    }, 2 );

    LUMEN_CHECK_TRUE( Emplaced.IsValid() );
    LUMEN_CHECK_EQ( *Emplaced, 42 );
}

/**
 * TUniquePtr<Type[]> — Arrays
 */

LUMEN_TEST( UniquePtr_Array_MakeUnique, ZeroInitializesEveryElement )
{
    auto Array = MakeUnique<Int32[]>( 3 );

    LUMEN_CHECK_TRUE( Array.IsValid() );
    LUMEN_CHECK_TRUE( Array.Get() != nullptr );

    for ( USize Index = 0; Index < 3; ++Index )
    {
        LUMEN_CHECK_EQ( Array[Index], 0 );
    }

    Array[0] = 10;
    Array[1] = 20;
    Array[2] = 30;

    LUMEN_CHECK_EQ( Array[0], 10 );
    LUMEN_CHECK_EQ( Array[1], 20 );
    LUMEN_CHECK_EQ( Array[2], 30 );
}

LUMEN_TEST( UniquePtr_Array_MakeUnique, DestroysEveryElementOnReset )
{
    FUniqueState State;
    auto Array = MakeUnique<FArrayElement[]>( 3 );

    for ( USize Index = 0; Index < 3; ++Index )
    {
        Array[Index].Bind( State );
    }

    LUMEN_CHECK_EQ( State.Alive, 3 );
    LUMEN_CHECK_EQ( State.Constructed, 3 );

    Array.Reset();

    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 3 );
}

LUMEN_TEST( UniquePtr_Array_Constructor, RawArrayTakesOwnership )
{
    FUniqueState State;

    {
        TUniquePtr<FArrayElement[]> Array( new FArrayElement[2] );

        for ( USize ElementIndex = 0; ElementIndex < 2; ++ElementIndex )
        {
            Array[ElementIndex].Bind( State );
        }

        LUMEN_CHECK_TRUE( Array.IsValid() );
        LUMEN_CHECK_EQ( Array[0].Value, 1 );
        LUMEN_CHECK_EQ( Array[1].Value, 1 );
        LUMEN_CHECK_EQ( State.Alive, 2 );
    }

    LUMEN_CHECK_EQ( State.Alive, 0 );
    LUMEN_CHECK_EQ( State.Destroyed, 2 );
}

LUMEN_TEST( UniquePtr_Array_Access, ElementsAreReadableAndWritable )
{
    auto Array = MakeUnique<Int32[]>( 2 );

    Array[0] = 5;
    Array[1] = 6;

    LUMEN_CHECK_EQ( Array[0], 5 );
    LUMEN_CHECK_EQ( Array[1], 6 );

    const auto &ConstView = Array;

    LUMEN_CHECK_EQ( ConstView[1], 6 );
}

LUMEN_TEST( UniquePtr_Array_Move, ConstructorTransfersOwnership )
{
    FUniqueState State;
    TUniquePtr<FArrayElement[]> Origin( new FArrayElement[2] );

    TUniquePtr<FArrayElement[]> Target( std::move( Origin ) );

    for ( USize ElementIndex = 0; ElementIndex < 2; ++ElementIndex )
    {
        Target[ElementIndex].Bind( State );
    }

    LUMEN_CHECK_TRUE( Target.IsValid() );
    LUMEN_CHECK_FALSE( Origin.IsValid() );
    LUMEN_CHECK_EQ( State.Alive, 2 );
    LUMEN_CHECK_EQ( State.Destroyed, 0 );
}

LUMEN_TEST( UniquePtr_Array_Move, NullAssignmentDestroysArray )
{
    FUniqueState State;
    TUniquePtr<FArrayElement[]> Array( new FArrayElement[2] );

    for ( USize ElementIndex = 0; ElementIndex < 2; ++ElementIndex )
    {
        Array[ElementIndex].Bind( State );
    }

    Array = nullptr;

    LUMEN_CHECK_FALSE( Array.IsValid() );
    LUMEN_CHECK_EQ( State.Destroyed, 2 );
    LUMEN_CHECK_EQ( State.Alive, 0 );
}

LUMEN_TEST( UniquePtr_Array_Ownership, ResetReplacesTheArray )
{
    FUniqueState State;
    TUniquePtr<FArrayElement[]> Array( new FArrayElement[2] );

    for ( USize ElementIndex = 0; ElementIndex < 2; ++ElementIndex )
    {
        Array[ElementIndex].Bind( State );
    }

    Array.Reset( new FArrayElement[2] );
    Array[0].Bind( State );
    Array[1].Bind( State );

    LUMEN_CHECK_EQ( State.Destroyed, 2 );
    LUMEN_CHECK_EQ( State.Alive, 2 );

    Array.Reset();

    LUMEN_CHECK_EQ( State.Destroyed, 4 );
    LUMEN_CHECK_EQ( State.Alive, 0 );
}

LUMEN_TEST( UniquePtr_Array_Ownership, ReleaseRelinquishesOwnership )
{
    FUniqueState State;
    TUniquePtr<FArrayElement[]> Array( new FArrayElement[2] );

    for ( USize ElementIndex = 0; ElementIndex < 2; ++ElementIndex )
    {
        Array[ElementIndex].Bind( State );
    }

    FArrayElement *Raw = Array.Release();

    LUMEN_CHECK_FALSE( Array.IsValid() );
    LUMEN_CHECK_TRUE( Raw != nullptr );
    LUMEN_CHECK_EQ( State.Alive, 2 );

    delete[] Raw;

    LUMEN_CHECK_EQ( State.Destroyed, 2 );
    LUMEN_CHECK_EQ( State.Alive, 0 );
}

} // namespace Lumen
