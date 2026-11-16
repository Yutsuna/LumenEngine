/**
 * @file RHIObjectPoolTests.cpp
 * @brief Tests fixed-slot CPU object reuse.
 */

#include <Memory/RHIObjectPool.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

/**
 * Private
 */

namespace
{

    struct FTestPayload
    {
        explicit FTestPayload ( UInt32 InValue ) noexcept : Value( InValue )
        {
        }
        UInt32 Value;
    };

} // namespace

LUMEN_TEST( RHIObjectPool, ConstructsDestroysAndReusesSlots )
{
    TRHIObjectPool<FTestPayload> Pool( 2U );
    FTestPayload *First  = Pool.Create( 7U );
    FTestPayload *Second = Pool.Create( 9U );
    LUMEN_CHECK_NOT_EQ( First, nullptr );
    LUMEN_CHECK_NOT_EQ( Second, nullptr );
    LUMEN_CHECK_EQ( Second->Value, 9U );
    LUMEN_CHECK_EQ( Pool.Create( 11U ), nullptr );
    Pool.Destroy( First );
    LUMEN_CHECK_EQ( Pool.Create( 13U ), First );
    Pool.Destroy( First );
    Pool.Destroy( Second );
}

} // namespace Lumen
