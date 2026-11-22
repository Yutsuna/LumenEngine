
#include <Concepts/ConvertibleTo.hpp>
#include <Tests/UnitTest.hpp>

#include <String/String.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** @brief Helper with an implicit conversion operator to Int32. */
    struct FImplicitToInt
    {
        constexpr operator Int32 () const noexcept
        {
            return 42;
        }
    };

    /** @brief Helper with an explicit conversion operator to Int32. */
    struct FExplicitToInt
    {
        explicit constexpr operator Int32 () const noexcept
        {
            return 42;
        }
    };

    /** @brief Helper whose Int32 constructor is explicit, so Int32 is not implicitly convertible to it. */
    struct FExplicitFromInt
    {
        explicit constexpr FExplicitFromInt ( Int32 ) noexcept
        {
        }
    };

    struct FBase
    {
        /* */
    };

    struct FDerived : FBase
    {
        /* */
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_ConvertibleTo, TConvertibleTo )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<Int32, Float64> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<AnsiChar, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<const AnsiChar *, FString> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<FString, FString> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<FDerived *, FBase *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<Int32 *, const Int32 *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<Int32 &, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<NullPtrType, Int32 *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<FImplicitToInt, Int32> ));

        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<const Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CConvertibleTo<Int32 &, const Int32 &> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<FBase *, FDerived *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<const Int32 *, Int32 *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<const Int32 &, Int32 &> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<NullPtrType, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<FExplicitToInt, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<Int32, FExplicitFromInt> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<FString, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<Int32 *, Float64 *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<void, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CConvertibleTo<Int32, void> ));
    }

} // namespace

} // namespace Lumen
