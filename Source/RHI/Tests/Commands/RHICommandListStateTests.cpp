/**
 * @file RHICommandListStateTests.cpp
 * @brief Tests command-list lifecycle transitions.
 */

#include <RHI/Commands/RHICommandListState.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        LUMEN_TEST( RHICommandListState, WalksRecordingRenderingSubmissionAndReset )
        {
            ERHICommandListState State = ERHICommandListState::Initial;
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Recording ).HasValue() );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Rendering ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( RHITransitionCommandList( State, ERHICommandListState::Executable ) );
            LUMEN_CHECK_EQ( State, ERHICommandListState::Rendering );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Recording ).HasValue() );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Executable ).HasValue() );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Pending ).HasValue() );
            LUMEN_CHECK_HAS_ERROR( RHITransitionCommandList( State, ERHICommandListState::Initial ) );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Executable ).HasValue() );
            LUMEN_CHECK_TRUE( RHITransitionCommandList( State, ERHICommandListState::Initial ).HasValue() );
        }

    } // namespace

} // namespace RHI

} // namespace Lumen
