
#include <Concepts/Sortable.hpp>
#include <Tests/UnitTest.hpp>

#include <Memory/Span.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

#include <forward_list>
#include <initializer_list>
#include <list>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** @brief Strict-weak-order predicate for Int32 elements. */
    struct FAscendingIntPredicate
    {
        constexpr Bool operator()( Int32 Left, Int32 Right ) const noexcept
        {
            return Left < Right;
        }
    };

    /** @brief Element whose copy and move operations are deleted, making its range non-sortable. */
    struct FNonMovableElement
    {
        FNonMovableElement () noexcept                              = default;
        FNonMovableElement ( const FNonMovableElement & )           = delete;
        FNonMovableElement &operator=( const FNonMovableElement & ) = delete;
        FNonMovableElement ( FNonMovableElement && )                = delete;
        FNonMovableElement &operator=( FNonMovableElement && )      = delete;
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_Sortable, TSortable )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<TVector<Int32>, std::less<>> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<TVector<Int32>, std::greater<>> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<TVector<Int32>, FAscendingIntPredicate> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<TArray<Int32, 4>, std::less<>> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<Int32[4], std::less<>> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSortableRange<TSpan<Int32>, std::less<>> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<TVector<Int32>, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<TVector<FNonMovableElement>, std::less<>> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<std::forward_list<Int32>, std::less<>> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<std::list<Int32>, std::less<>> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<std::initializer_list<Int32>, std::less<>> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<Int32, std::less<>> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSortableRange<NullPtrType, std::less<>> ));
    }

} // namespace

} // namespace Lumen
