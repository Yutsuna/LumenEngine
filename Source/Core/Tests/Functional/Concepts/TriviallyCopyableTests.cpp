
#include <Concepts/TriviallyCopyable.hpp>
#include <Tests/UnitTest.hpp>

#include <String/String.hpp>
#include <Templates/Array.hpp>
#include <Templates/Vector.hpp>

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
        FirstValue,
        SecondValue,
    };

    struct FTrivialData
    {
        Int32 Value;
        Float64 Weight;
    };

    /** @brief Helper with a user-defined copy constructor, therefore not trivially copyable. */
    struct FNonTrivialCopy
    {
        FNonTrivialCopy () noexcept                               = default;
        FNonTrivialCopy ( const FNonTrivialCopy &Other ) noexcept = default;

        Int32 Value = 0;
    };

    struct FNonTrivialDestructor
    {
        ~FNonTrivialDestructor () noexcept = default;
    };

    struct FNonTrivialMove
    {
        FNonTrivialMove ( FNonTrivialMove && ) noexcept = default;
    };

    /**
     * Tests
     */

    LUMEN_TEST( Concepts_TriviallyCopyable, TTriviallyCopyable )
    {
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<Float64> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<Bool> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<Int32 *> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<NullPtrType> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<ETestEnum> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<FTrivialData> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<TArray<Int32, 4>> ));

        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<const Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<volatile Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<const volatile Int32> ));
        LUMEN_CHECK_TRUE( ( Concepts::CTriviallyCopyable<Int32[4]> ));

        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<FNonTrivialCopy> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<FNonTrivialDestructor> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<FNonTrivialMove> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<FString> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<TVector<Int32>> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<Int32 &> ));
        LUMEN_CHECK_TRUE( ( not Concepts::CTriviallyCopyable<void> ));
    }

} // namespace

} // namespace Lumen
