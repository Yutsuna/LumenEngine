
#include <Concepts/IsStringLike.hpp>
#include <Tests/UnitTest.hpp>

#include <String/StringView.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** @brief Custom type implicitly convertible to FString. */
    struct FCustomStringLike
    {
        operator FString () const
        {
            return FString{};
        }
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsStringLike, TIsStringLike )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<FString> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<const FString &> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<const AnsiChar *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<AnsiChar *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<AnsiChar[4]> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsStringLike<FCustomStringLike> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<FStringView> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<const volatile AnsiChar *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<WideChar *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<FString *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<Float64> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<NullPtrType> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsStringLike<void> ));
    }

} // namespace

} // namespace Lumen
