
#include <Concepts/IsIntegral.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * Data Tests
     */

    /** NOLINTNEXTLINE(performance-enum-size) */
    enum ETestEnum
    {
        AValue,
        BValue,
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsIntegral, TIsIntegral )
    {

        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<Int8> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<UInt8> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<Byte> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<AnsiChar> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<WideChar> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<Int16> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<UInt64> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<Bool> );
        LUMEN_CHECK_TRUE( Concepts::TIsIntegralT<const volatile Int32> );

        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<Float32> );
        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<Float64> );
        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<NullPtrType> );
        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<Int32 *> );
        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<Int32 &> );
        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<TVoidT<>> );

        LUMEN_CHECK_TRUE( not Concepts::TIsIntegralT<ETestEnum> );
        LUMEN_CHECK_TRUE( not Concepts::CIsIntegral<ETestEnum> );
    }

} // namespace

} // namespace Lumen
