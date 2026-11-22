
#include <Concepts/IsInvocable.hpp>
#include <Tests/UnitTest.hpp>

#include <String/String.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    Int32 AddInts ( Int32 Left, Int32 Right ) noexcept
    {
        return Left + Right;
    }

    /** @brief Function object exercising the callable-object path of Invoke(). */
    struct FAddFunctor
    {
        constexpr Int32 operator()( Int32 Left, Int32 Right ) const noexcept
        {
            return Left + Right;
        }
    };

    /** @brief Helper exposing both a member function and member data for pointer-to-member invocation. */
    struct FWidget
    {
        [[nodiscard]] Int32 GetValue () const noexcept
        {
            return 42;
        }

        Int32 Value = 0;
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsInvocable, TIsInvocable )
    {
        LUMEN_CHECK_EQ( AddInts( 2, 3 ), 5 );

        LUMEN_CHECK_TRUE( ( Concepts::CIsInvocable<decltype( &AddInts ), Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsInvocable<FAddFunctor, Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsInvocable<decltype( &FWidget::GetValue ), const FWidget &> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsInvocable<decltype( &FWidget::Value ), FWidget &> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<decltype( &AddInts )> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<decltype( &AddInts ), Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<decltype( &AddInts ), FString, FString> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<FAddFunctor, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<decltype( &FWidget::GetValue ), Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<decltype( &FWidget::Value ), Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsInvocable<NullPtrType, Int32> ));

        LUMEN_CHECK_TRUE( ( TIsInvocable<decltype( &AddInts ), Int32, Int32>::Value ) );
        LUMEN_CHECK_TRUE( ( TIsInvocableV<decltype( &AddInts ), Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( TIsInvocableV<FAddFunctor, Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( TIsInvocableV<decltype( &FWidget::GetValue ), const FWidget &> ));
        LUMEN_CHECK_TRUE( ( not TIsInvocable<decltype( &AddInts )>::Value ) );
        LUMEN_CHECK_TRUE( ( not TIsInvocableV<decltype( &AddInts ), Int32> ));
        LUMEN_CHECK_TRUE( ( not TIsInvocableV<Int32, Int32> ));
    }

} // namespace

} // namespace Lumen
