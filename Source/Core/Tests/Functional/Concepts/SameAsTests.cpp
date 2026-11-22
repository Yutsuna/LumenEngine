
#include <Concepts/SameAs.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    using FAliasInt = Int32;

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_SameAs, TSameAs )
    {
        LUMEN_CHECK_TRUE( ( Internal::TIsSame<Int32, Int32>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<FAliasInt, Int32> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<Byte, UInt8> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<void, TVoidT<>> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<const volatile Int32, const volatile Int32> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<Int32 &, Int32 &> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<Int32 &&, Int32 &&> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsSameV<const Int32 &, const Int32 &> ));

        LUMEN_CHECK_TRUE( ( not Internal::TIsSame<Int32, Float32>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<Int32, Float32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<Bool, Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<const Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<volatile Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<Int32 &, Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<Int32 &&, Int32 &> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<Int32 *, const Int32 *> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsSameV<void, NullPtrType> ));

        LUMEN_CHECK_TRUE( ( Concepts::CSameAs<Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSameAs<FAliasInt, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSameAs<const volatile Int32, const volatile Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CSameAs<Int32 &, Int32 &> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSameAs<Int32, Float32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSameAs<const Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CSameAs<Int32 *, const Int32 *> ));

        LUMEN_CHECK_TRUE( ( Concepts::CIsSame<Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsSame<FAliasInt, Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsSame<const volatile Int32, const volatile Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsSame<Int32 &, Int32 &> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsSame<Int32, Float32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsSame<const Int32, Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsSame<Int32 *, const Int32 *> ));
    }

} // namespace

} // namespace Lumen
