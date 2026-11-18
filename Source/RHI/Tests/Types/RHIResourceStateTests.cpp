/**
 * @file RHIResourceStateTests.cpp
 * @brief Tests resource-state transition rules.
 */

#include <Tests/UnitTest.hpp>
#include <Types/RHIResourceState.hpp>

namespace Lumen
{

LUMEN_TEST( RHIResourceState, ValidatesStatesAndTransitions )
{
    LUMEN_CHECK_TRUE( IsValidRHIResourceState( ERHIResourceState::ShaderResource ) );
    LUMEN_CHECK_TRUE( IsValidRHIResourceTransition( ERHIResourceState::Undefined, ERHIResourceState::CopyDestination ) );
    LUMEN_CHECK_FALSE( IsValidRHIResourceTransition( ERHIResourceState::Common, ERHIResourceState::Common ) );
    LUMEN_CHECK_FALSE( IsValidRHIResourceState( static_cast<ERHIResourceState>( 1U << 31U ) ) );
}

} // namespace Lumen
