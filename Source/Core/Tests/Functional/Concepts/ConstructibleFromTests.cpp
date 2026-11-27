
#include <Concepts/ConstructibleFrom.hpp>
#include <Tests/UnitTest.hpp>

#include <String/String.hpp>
#include <String/StringView.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** @brief Aggregate-like helper with a two-argument constructor and no default constructor. */
    struct FPoint
    {
        Int32 X;
        Int32 Y;

        constexpr FPoint ( Int32 InX, Int32 InY ) noexcept : X( InX ), Y( InY )
        {
            /* */
        }
    };

    /** @brief Helper whose constructor is explicit: construction is allowed, implicit conversion is not. */
    struct FExplicitFromInt
    {
        explicit constexpr FExplicitFromInt ( const Int32 LUMEN_UNUSED InValue ) noexcept
        {
            /* */
        }
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_ConstructibleFrom, TConstructibleFrom )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<const Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FString, const AnsiChar *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FString, FStringView> ));

        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FPoint, Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FPoint, const FPoint &> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FPoint, FPoint &&> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<FExplicitFromInt, Int32> ));

        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<Int32 &, Int32 &> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConstructibleFrom<const Int32 &, Int32> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CConstructibleFrom<FPoint> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConstructibleFrom<FPoint, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConstructibleFrom<Int32 &, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConstructibleFrom<Int32, FString> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConstructibleFrom<Int32, void> ));
    }

} // namespace

} // namespace Lumen
