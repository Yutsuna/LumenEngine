
#include <Concepts/IsBool.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_IsBool, TIsBool )
    {
        const Internal::TTrueType TrueTypeValue;
        const Internal::TFalseType FalseTypeValue;

        LUMEN_CHECK_TRUE( ( Internal::TTrueType::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TFalseType::Value ) );
        LUMEN_CHECK_TRUE( ( static_cast<Bool>( TrueTypeValue ) ) );
        LUMEN_CHECK_TRUE( ( not static_cast<Bool>( FalseTypeValue ) ) );

        LUMEN_CHECK_TRUE( ( Internal::TBooleanConstant<true>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TBooleanConstantV<true> ));
        LUMEN_CHECK_TRUE( ( not Internal::TBooleanConstant<false>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TBooleanConstantV<false> ));

        LUMEN_CHECK_TRUE( ( Internal::TIsBool<Bool>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsBool<const Bool>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsBool<volatile Bool>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsBool<const volatile Bool>::Value ) );
        LUMEN_CHECK_TRUE( ( Internal::TIsBoolV<Bool> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsBoolV<const Bool> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsBoolV<volatile Bool> ));
        LUMEN_CHECK_TRUE( ( Internal::TIsBoolV<const volatile Bool> ));

        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<Int32>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<Float32>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<NullPtrType>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<Bool *>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<Bool &>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBool<TVoidT<>>::Value ) );
        LUMEN_CHECK_TRUE( ( not Internal::TIsBoolV<Int32> ));
        LUMEN_CHECK_TRUE( ( not Internal::TIsBoolV<Bool *> ));

        LUMEN_CHECK_TRUE( ( Concepts::CIsBool<Bool> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsBool<const Bool> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsBool<volatile Bool> ));
        LUMEN_CHECK_TRUE( ( Concepts::CIsBool<const volatile Bool> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<Int32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<Float32> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<NullPtrType> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<Bool *> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<Bool &> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<TVoidT<>> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CIsBool<Internal::TTrueType> ));
    }

} // namespace

} // namespace Lumen
