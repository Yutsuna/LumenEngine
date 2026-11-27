
#include <Concepts/IsVoid.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsVoid, TIsVoid )
    {
        LUMEN_CHECK_TRUE( ( Internal::TIsVoid<void>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsVoid<const void>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsVoid<volatile void>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsVoid<const volatile void>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsVoidV<void> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsVoidV<const void> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsVoidV<volatile void> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsVoidV<const volatile void> ));

        LUMEN_CHECK_TRUE( ( not Internal::TIsVoid<Int32>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsVoid<Float32>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsVoid<void *>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsVoid<const void *>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsVoidV<Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsVoidV<void *> ));

        LUMEN_CHECK_TRUE( ( Concepts::CIsVoid<void> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsVoid<const void> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsVoid<volatile void> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsVoid<const volatile void> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsVoid<TVoidT<>> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CIsVoid<Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsVoid<NullPtrType> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsVoid<void *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsVoid<const volatile void *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsVoid<Internal::TTrueType> ));
    }

} // namespace

} // namespace Lumen
