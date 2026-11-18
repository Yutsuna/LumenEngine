/**
 * @file RHIMultisampleStateTests.cpp
 * @brief Tests multisample count and mask validation.
 */

#include <RHI/Pipelines/RHIMultisampleState.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIMultisampleState, RequiresKnownPowerOfTwoCountAndNonZeroMask )
        {
            FRHIMultisampleState State;
            LUMEN_CHECK_TRUE( State.Validate().HasValue() );
            State.Count = ERHISampleCount::Count8;
            LUMEN_CHECK_TRUE( State.Validate().HasValue() );
            State.SampleMask = 0U;
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.SampleMask = 0xFFFFFFFFU;
            State.Count      = static_cast<ERHISampleCount>( 3U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
            State.Count = static_cast<ERHISampleCount>( 128U );
            LUMEN_CHECK_HAS_ERROR( State.Validate() );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
