/**
 * @file SharedPtrTests.cpp
 * @brief Functional unit tests for TSharedRef, TSharedPtr, MakeShared/MakeShareable and the static casts.
 */

#include <Memory/SharedPtr.hpp>
#include <Tests/UnitTest.hpp>

#include <type_traits>
#include <utility>

namespace Lumen
{

namespace
{

    /**
     * Private
     */

    /** @brief Lifetime bookkeeping shared by every test of this file. */
    struct FSharedState
    {
        Int32 Alive       = 0;
        Int32 Constructed = 0;
        Int32 Destroyed   = 0;
    };

    /** @brief Value object whose lifetime is traced into an FSharedState. */
    class FSharedLifetimeObject final
    {
    public:

        explicit FSharedLifetimeObject ( FSharedState &InState ) noexcept : State( &InState ), Value( 0 )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FSharedLifetimeObject ( FSharedState &InState, const Int32 InValue ) noexcept : State( &InState ), Value( InValue )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        FSharedLifetimeObject ( const FSharedLifetimeObject &InOther ) noexcept : State( InOther.State ), Value( InOther.Value )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        ~FSharedLifetimeObject () noexcept
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

        FSharedState *State;
        Int32 Value;
    };

    /** @brief Polymorphic base used for upcasting and static casting tests. */
    struct FSharedBase
    {
        virtual ~FSharedBase () noexcept = default;

        [[nodiscard]] virtual Int32 Id () const noexcept
        {
            return 1;
        }
    };

    /** @brief Concrete leaf observed through base pointers, with lifetime tracing. */
    struct FSharedDerived final : FSharedBase
    {
        FSharedDerived ( FSharedState &InState, const Int32 InExtra ) noexcept : State( &InState ), Extra( InExtra )
        {
            ++State->Alive;
            ++State->Constructed;
        }

        ~FSharedDerived () noexcept override
        {
            --State->Alive;
            ++State->Destroyed;
        }

        [[nodiscard]] Int32 Id () const noexcept override
        {
            return 2;
        }

        [[nodiscard]] Int32 GetExtra () const noexcept
        {
            return Extra;
        }

        FSharedState *State;
        Int32 Extra;
    };

    static_assert( not std::is_default_constructible_v<TSharedRef<FSharedLifetimeObject>> );
    static_assert( not std::is_constructible_v<TSharedRef<FSharedLifetimeObject>, std::nullptr_t> );
    static_assert( std::is_copy_constructible_v<TSharedRef<FSharedLifetimeObject>> );
    static_assert( std::is_move_constructible_v<TSharedRef<FSharedLifetimeObject>> );
    static_assert( std::is_default_constructible_v<TSharedPtr<FSharedLifetimeObject>> );
    static_assert( std::is_copy_constructible_v<TSharedPtr<FSharedLifetimeObject>> );
    static_assert( std::is_move_constructible_v<TSharedPtr<FSharedLifetimeObject>> );
    static_assert( not std::is_constructible_v<TSharedPtr<FSharedDerived>, const TSharedRef<FSharedBase> &> );

    /**
     * SharedRef — Construction
     */

    LUMEN_TEST( SharedRef_Constructor, MakeSharedBuildsObjectExactlyOnce )
    {
        FSharedState State;

        {
            const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State );

            LUMEN_CHECK_EQ( State.Alive, 1 );
            LUMEN_CHECK_EQ( State.Constructed, 1 );
            LUMEN_CHECK_EQ( State.Destroyed, 0 );
            LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 1U );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedRef_Constructor, MakeSharedForwardsConstructorArguments )
    {
        FSharedState State;
        const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State, 21 );

        LUMEN_CHECK_EQ( Ref->GetValue(), 21 );
        LUMEN_CHECK_EQ( State.Constructed, 1 );
    }

    LUMEN_TEST( SharedRef_Constructor, MakeShareableProxyTakesOwnership )
    {
        FSharedState State;

        {
            const TSharedRef<FSharedLifetimeObject> Ref = MakeShareable( new FSharedLifetimeObject( State ) );

            LUMEN_CHECK_EQ( State.Alive, 1 );
            LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 1U );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedRef_Constructor, UpcastsFromDerived )
    {
        FSharedState State;

        {
            const TSharedRef<FSharedBase> Base = MakeShared<FSharedDerived>( State, 7 );

            LUMEN_CHECK_EQ( Base->Id(), 2 );
            LUMEN_CHECK_EQ( Base.GetSharedReferenceCount(), 1U );

            const TSharedPtr<FSharedBase> Shared = Base;

            LUMEN_CHECK_TRUE( Shared.Get() == Base.Get() );
            LUMEN_CHECK_EQ( Base.GetSharedReferenceCount(), 2U );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedRef_Access, DereferenceAndMemberAccess )
    {
        FSharedState State;
        const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State, 5 );

        FSharedLifetimeObject &Object = *Ref;
        Object.SetValue( 12 );

        LUMEN_CHECK_EQ( Ref->GetValue(), 12 );
        LUMEN_CHECK_EQ( Object.GetValue(), 12 );
        LUMEN_CHECK_TRUE( Ref.Get() == &Object );

        Ref->SetValue( 9 );
        LUMEN_CHECK_EQ( ( *Ref ).GetValue(), 9 );
    }

    /**
     * SharedRef — Reference Counting
     */

    LUMEN_TEST( SharedRef_ReferenceCount, CopiesShareTheSameObject )
    {
        FSharedState State;
        const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State );

        {
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization) — copy under test
            const TSharedRef<FSharedLifetimeObject> Copy( Ref );

            LUMEN_CHECK_EQ( Copy.GetSharedReferenceCount(), 2U );
            LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 2U );
            LUMEN_CHECK_TRUE( Copy.Get() == Ref.Get() );
            LUMEN_CHECK_EQ( State.Alive, 1 );
        }

        LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 1U );
        LUMEN_CHECK_EQ( State.Alive, 1 );
        LUMEN_CHECK_EQ( State.Destroyed, 0 );
    }

    LUMEN_TEST( SharedRef_ReferenceCount, MoveTransfersTheSingleReference )
    {
        FSharedState State;

        {
            TSharedRef<FSharedLifetimeObject> Origin = MakeShared<FSharedLifetimeObject>( State );

            const TSharedRef<FSharedLifetimeObject> Target( std::move( Origin ) );

            LUMEN_CHECK_EQ( Target.GetSharedReferenceCount(), 1U );
            LUMEN_CHECK_EQ( State.Alive, 1 );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedRef_ReferenceCount, CopyOfDerivedSharesCountWithBase )
    {
        FSharedState State;
        const TSharedRef<FSharedBase> Base = MakeShared<FSharedDerived>( State, 3 );

        // NOLINTNEXTLINE(performance-unnecessary-copy-initialization) — copy under test
        const TSharedRef<FSharedBase> Copy( Base );

        LUMEN_CHECK_EQ( Copy.GetSharedReferenceCount(), 2U );
        LUMEN_CHECK_EQ( Base.GetSharedReferenceCount(), 2U );
    }

    /**
     * SharedRef — Assignment
     */

    LUMEN_TEST( SharedRef_Assignment, CopyAssignReplacesManagedObject )
    {
        FSharedState FirstState;
        FSharedState SecondState;
        TSharedRef<FSharedLifetimeObject> First        = MakeShared<FSharedLifetimeObject>( FirstState, 1 );
        const TSharedRef<FSharedLifetimeObject> Second = MakeShared<FSharedLifetimeObject>( SecondState, 2 );

        First = Second;

        LUMEN_CHECK_TRUE( First.Get() == Second.Get() );
        LUMEN_CHECK_EQ( First->GetValue(), 2 );
        LUMEN_CHECK_EQ( First.GetSharedReferenceCount(), 2U );
        LUMEN_CHECK_EQ( Second.GetSharedReferenceCount(), 2U );

        LUMEN_CHECK_EQ( FirstState.Alive, 0 );
        LUMEN_CHECK_EQ( FirstState.Destroyed, 1 );
        LUMEN_CHECK_EQ( SecondState.Alive, 1 );
    }

    LUMEN_TEST( SharedRef_Assignment, MoveAssignTakesOwnershipAndEmptiesSource )
    {
        FSharedState FirstState;
        FSharedState SecondState;
        TSharedRef<FSharedLifetimeObject> First  = MakeShared<FSharedLifetimeObject>( FirstState, 1 );
        TSharedRef<FSharedLifetimeObject> Second = MakeShared<FSharedLifetimeObject>( SecondState, 2 );

        First = std::move( Second );

        LUMEN_CHECK_EQ( First->GetValue(), 2 );
        LUMEN_CHECK_EQ( First.GetSharedReferenceCount(), 1U );

        LUMEN_CHECK_EQ( FirstState.Alive, 0 );
        LUMEN_CHECK_EQ( FirstState.Destroyed, 1 );
        LUMEN_CHECK_EQ( SecondState.Alive, 1 );
        LUMEN_CHECK_EQ( SecondState.Destroyed, 0 );
    }

    LUMEN_TEST( SharedRef_Assignment, SelfAssignmentIsHarmless )
    {
        FSharedState State;
        TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State );

        Ref = Ref;

        TSharedRef<FSharedLifetimeObject> &SelfSource = Ref;
        Ref                                           = std::move( SelfSource );

        LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 1U );
        LUMEN_CHECK_EQ( State.Alive, 1 );
        LUMEN_CHECK_EQ( State.Destroyed, 0 );
    }

    /**
     * Static Casts
     */

    LUMEN_TEST( StaticCast_SharedRef, DowncastToConcreteType )
    {
        FSharedState State;
        const TSharedRef<FSharedBase> Base = MakeShared<FSharedDerived>( State, 77 );

        const TSharedRef<FSharedDerived> Down = StaticCastSharedRef<FSharedDerived>( Base );

        LUMEN_CHECK_EQ( Down->GetExtra(), 77 );
        LUMEN_CHECK_TRUE( Down.Get() == Base.Get() );
        LUMEN_CHECK_EQ( Down.GetSharedReferenceCount(), 2U );

        LUMEN_CHECK_EQ( State.Alive, 1 );
        LUMEN_CHECK_EQ( State.Destroyed, 0 );
    }

    LUMEN_TEST( StaticCast_SharedPtr, DowncastToConcreteType )
    {
        FSharedState State;
        const TSharedPtr<FSharedBase> Base = MakeShared<FSharedDerived>( State, 44 );

        const TSharedPtr<FSharedDerived> Down = StaticCastSharedPtr<FSharedDerived>( Base );

        LUMEN_CHECK_TRUE( Down.IsValid() );
        LUMEN_CHECK_EQ( Down->GetExtra(), 44 );
        LUMEN_CHECK_TRUE( Down.Get() == Base.Get() );
    }

    LUMEN_TEST( StaticCast_SharedPtr, InvalidPointerStaysInvalid )
    {
        const TSharedPtr<FSharedBase> Empty;
        const TSharedPtr<FSharedDerived> Down = StaticCastSharedPtr<FSharedDerived>( Empty );

        LUMEN_CHECK_FALSE( Down.IsValid() );
        LUMEN_CHECK_TRUE( Down.Get() == nullptr );
    }

    /**
     * SharedPtr — Construction
     */

    LUMEN_TEST( SharedPtr_Constructor, DefaultAndNullPointersStayInvalid )
    {
        TSharedPtr<FSharedLifetimeObject> Empty;
        const TSharedPtr<FSharedLifetimeObject> Null( nullptr );

        LUMEN_CHECK_FALSE( Empty.IsValid() );
        LUMEN_CHECK_FALSE( Null.IsValid() );
        LUMEN_CHECK_TRUE( Empty.Get() == nullptr );
        LUMEN_CHECK_TRUE( Null.Get() == nullptr );

        Empty.Reset();
        LUMEN_CHECK_FALSE( Empty.IsValid() );
    }

    LUMEN_TEST( SharedPtr_Constructor, FromSharedRefSharesTheSameObject )
    {
        FSharedState State;
        const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State, 8 );

        {
            const TSharedPtr<FSharedLifetimeObject> Ptr = Ref;

            LUMEN_CHECK_TRUE( Ptr.IsValid() );
            LUMEN_CHECK_TRUE( Ptr.Get() == Ref.Get() );
            LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 2U );
        }

        LUMEN_CHECK_EQ( Ref.GetSharedReferenceCount(), 1U );
        LUMEN_CHECK_EQ( State.Alive, 1 );
    }

    LUMEN_TEST( SharedPtr_Constructor, UpcastFromSharedRefOfDerived )
    {
        FSharedState State;

        {
            const TSharedPtr<FSharedBase> Ptr = MakeShared<FSharedDerived>( State, 31 );

            LUMEN_CHECK_TRUE( Ptr.IsValid() );
            LUMEN_CHECK_EQ( Ptr->Id(), 2 );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedPtr_Constructor, MakeShareableDestroysOnLastRelease )
    {
        FSharedState State;

        {
            const TSharedPtr<FSharedLifetimeObject> Ptr = MakeShareable( new FSharedLifetimeObject( State, 13 ) );

            LUMEN_CHECK_TRUE( Ptr.IsValid() );
            LUMEN_CHECK_EQ( Ptr->GetValue(), 13 );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedPtr_Constructor, NullProxiedPointerStaysInvalid )
    {
        const FSharedState State;
        const TSharedPtr<FSharedLifetimeObject> Ptr = MakeShareable( static_cast<FSharedLifetimeObject *>( nullptr ) );

        LUMEN_CHECK_FALSE( Ptr.IsValid() );
        LUMEN_CHECK_TRUE( Ptr.Get() == nullptr );
        LUMEN_CHECK_EQ( State.Alive, 0 );
    }

    /**
     * SharedPtr — Copy / Move
     */

    LUMEN_TEST( SharedPtr_Lifetime, CopyKeepsObjectAliveForAllOwners )
    {
        FSharedState State;

        {
            const TSharedPtr<FSharedLifetimeObject> First = MakeShared<FSharedLifetimeObject>( State );

            {
                // NOLINTNEXTLINE(performance-unnecessary-copy-initialization) — copy under test
                const TSharedPtr<FSharedLifetimeObject> Second( First );

                LUMEN_CHECK_TRUE( First.Get() == Second.Get() );
                LUMEN_CHECK_EQ( State.Alive, 1 );
            }

            LUMEN_CHECK_TRUE( First.IsValid() );
            LUMEN_CHECK_EQ( State.Alive, 1 );
            LUMEN_CHECK_EQ( State.Destroyed, 0 );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedPtr_Lifetime, MoveConstructorTransfersOwnership )
    {
        FSharedState State;

        {
            TSharedPtr<FSharedLifetimeObject> Origin = MakeShareable( new FSharedLifetimeObject( State, 6 ) );

            const TSharedPtr<FSharedLifetimeObject> Target( std::move( Origin ) );

            LUMEN_CHECK_TRUE( Target.IsValid() );
            LUMEN_CHECK_EQ( Target->GetValue(), 6 );
            // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move) — moved-from check
            LUMEN_CHECK_TRUE( Origin.Get() == nullptr );
            // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move) — moved-from check
            LUMEN_CHECK_FALSE( Origin.IsValid() );

            const TSharedPtr<FSharedLifetimeObject> Ghost( std::move( Origin ) );

            LUMEN_CHECK_FALSE( Ghost.IsValid() );
        }

        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    LUMEN_TEST( SharedPtr_Lifetime, CopyOfMovedOutSourceStaysEmpty )
    {
        FSharedState State;
        TSharedPtr<FSharedLifetimeObject> Origin = MakeShareable( new FSharedLifetimeObject( State ) );

        const TSharedPtr<FSharedLifetimeObject> Target( std::move( Origin ) );
        // Copying the moved-from source is the test subject: verify the copy stays empty.
        // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move) — moved-from check
        const TSharedPtr<FSharedLifetimeObject> Copy( Origin ); // NOLINT(performance-unnecessary-copy-initialization)

        LUMEN_CHECK_FALSE( Copy.IsValid() );
        // NOLINTNEXTLINE(bugprone-use-after-move,hicpp-invalid-access-moved,clang-analyzer-cplusplus.Move) — moved-from check
        LUMEN_CHECK_TRUE( Origin.Get() == nullptr );
        LUMEN_CHECK_TRUE( Target.IsValid() );
    }

    LUMEN_TEST( SharedPtr_Lifetime, CopyAssignmentReplacesManagedObject )
    {
        FSharedState FirstState;
        FSharedState SecondState;
        TSharedPtr<FSharedLifetimeObject> First        = MakeShareable( new FSharedLifetimeObject( FirstState, 1 ) );
        const TSharedPtr<FSharedLifetimeObject> Second = MakeShareable( new FSharedLifetimeObject( SecondState, 2 ) );

        First = Second;

        LUMEN_CHECK_TRUE( First.Get() == Second.Get() );
        LUMEN_CHECK_EQ( First->GetValue(), 2 );

        LUMEN_CHECK_EQ( FirstState.Destroyed, 1 );
        LUMEN_CHECK_EQ( SecondState.Alive, 1 );
    }

    LUMEN_TEST( SharedPtr_Lifetime, AssignmentFromSharedRefTakesReference )
    {
        FSharedState State;
        TSharedPtr<FSharedLifetimeObject> Ptr;

        Ptr = MakeShared<FSharedLifetimeObject>( State, 3 );

        LUMEN_CHECK_TRUE( Ptr.IsValid() );
        LUMEN_CHECK_EQ( Ptr->GetValue(), 3 );
    }

    LUMEN_TEST( SharedPtr_Lifetime, SelfAssignmentIsHarmless )
    {
        FSharedState State;
        TSharedPtr<FSharedLifetimeObject> Ptr = MakeShared<FSharedLifetimeObject>( State );

        Ptr = Ptr;

        const TSharedPtr<FSharedLifetimeObject> &SelfSource = Ptr;
        Ptr                                                 = SelfSource; // no move assignment; self-copy is the only path

        LUMEN_CHECK_TRUE( Ptr.IsValid() );

        TSharedPtr<FSharedLifetimeObject> Empty;

        Empty = Empty;
        LUMEN_CHECK_FALSE( Empty.IsValid() );

        LUMEN_CHECK_EQ( State.Alive, 1 );
        LUMEN_CHECK_EQ( State.Destroyed, 0 );
    }

    /**
     * SharedPtr — Access
     */

    LUMEN_TEST( SharedPtr_Access, DereferenceAndBoolConversion )
    {
        FSharedState State;
        const TSharedPtr<FSharedLifetimeObject> Valid = MakeShareable( new FSharedLifetimeObject( State, 4 ) );
        const TSharedPtr<FSharedLifetimeObject> Invalid;

        Valid->SetValue( 14 );

        LUMEN_CHECK_EQ( ( *Valid ).GetValue(), 14 );
        LUMEN_CHECK_TRUE( static_cast<Bool>( Valid ) );
        LUMEN_CHECK_FALSE( static_cast<Bool>( Invalid ) );
        LUMEN_CHECK_TRUE( Valid.IsValid() );
        LUMEN_CHECK_FALSE( Invalid.IsValid() );
        LUMEN_CHECK_TRUE( Invalid.Get() == nullptr );
    }

    LUMEN_TEST( SharedPtr_Access, ResetDestroysManagedObject )
    {
        FSharedState State;
        TSharedPtr<FSharedLifetimeObject> Ptr = MakeShareable( new FSharedLifetimeObject( State ) );

        Ptr.Reset();

        LUMEN_CHECK_FALSE( Ptr.IsValid() );
        LUMEN_CHECK_TRUE( Ptr.Get() == nullptr );
        LUMEN_CHECK_EQ( State.Alive, 0 );
        LUMEN_CHECK_EQ( State.Destroyed, 1 );

        Ptr.Reset();
        LUMEN_CHECK_EQ( State.Destroyed, 1 );
    }

    /**
     * SharedPtr — Comparisons
     */

    LUMEN_TEST( SharedPtr_Comparison, SameObjectComparesEqual )
    {
        FSharedState State;
        const TSharedPtr<FSharedLifetimeObject> First   = MakeShared<FSharedLifetimeObject>( State );
        const TSharedPtr<FSharedLifetimeObject> &Second = First;
        const TSharedPtr<FSharedLifetimeObject> Empty;

        LUMEN_CHECK_TRUE( First == Second );
        LUMEN_CHECK_FALSE( First != Second );
        LUMEN_CHECK_TRUE( First != Empty );
        LUMEN_CHECK_TRUE( Empty == Empty );
    }

    LUMEN_TEST( SharedPtr_Comparison, SharedRefComparesByObject )
    {
        FSharedState State;
        TSharedPtr<FSharedLifetimeObject> Ptr       = MakeShared<FSharedLifetimeObject>( State );
        const TSharedRef<FSharedLifetimeObject> Ref = MakeShared<FSharedLifetimeObject>( State );

        LUMEN_CHECK_TRUE( Ptr != Ref );
        LUMEN_CHECK_FALSE( Ptr == Ref );

        Ptr = Ref;

        LUMEN_CHECK_TRUE( Ptr == Ref );
        LUMEN_CHECK_FALSE( Ptr != Ref );
    }

    LUMEN_TEST( SharedPtr_Comparison, UpcastComparisonStillWorks )
    {
        FSharedState State;
        const TSharedPtr<FSharedBase> Base = MakeShared<FSharedDerived>( State, 1 );

        const TSharedPtr<FSharedDerived> Derived = StaticCastSharedPtr<FSharedDerived>( Base );

        LUMEN_CHECK_TRUE( Base == Derived );
        LUMEN_CHECK_FALSE( Base != Derived );
    }

} // namespace

} // namespace Lumen
