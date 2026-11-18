/**
 * @file RHIObjectPoolTests.cpp
 * @brief Tests fixed-slot CPU object reuse.
 */

#include <Memory/RHIObjectPool.hpp>
#include <Templates/Optional.hpp>
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

/**
 * Public
 */

LUMEN_TEST( RHIObjectPool, ConstructsDestroysAndReusesSlots )
{
    TRHIObjectPool<FTestPayload> Pool( 2U );
    const TOptional<FTestPayload *> First  = Pool.Create( 7U );
    const TOptional<FTestPayload *> Second = Pool.Create( 9U );
    LUMEN_CHECK_HAS_VALUE( First );
    LUMEN_CHECK_HAS_VALUE( Second );
    LUMEN_CHECK_EQ( ( *Second )->Value, 9U );
    LUMEN_CHECK_FALSE( Pool.Create( 11U ).HasValue() );
    Pool.Destroy( *First );
    const TOptional<FTestPayload *> Reused = Pool.Create( 13U );
    LUMEN_CHECK_HAS_VALUE( Reused );
    LUMEN_CHECK_EQ( *Reused, *First );
    Pool.Destroy( *Reused );
    Pool.Destroy( *Second );
}

LUMEN_TEST( RHIObjectPool, ExhaustionReturnsEmptyOptionalAndDestroyedSlotIsReused )
{
    TRHIObjectPool<FTestPayload> Pool( 1U );
    const TOptional<FTestPayload *> Only = Pool.Create( 21U );
    LUMEN_CHECK_HAS_VALUE( Only );
    LUMEN_CHECK_FALSE( Pool.Create( 22U ).HasValue() );
    LUMEN_CHECK_FALSE( Pool.Create( 23U ).HasValue() );
    Pool.Destroy( *Only );
    const TOptional<FTestPayload *> Reused = Pool.Create( 24U );
    LUMEN_CHECK_HAS_VALUE( Reused );
    LUMEN_CHECK_EQ( *Reused, *Only );
    LUMEN_CHECK_EQ( ( *Reused )->Value, 24U );
    LUMEN_CHECK_FALSE( Pool.Create( 25U ).HasValue() );
    Pool.Destroy( *Reused );
}

} // namespace Lumen
