/**
 * @file VulkanQueueTests.cpp
 * @brief Integration tests for the Vulkan V2 queue idle and host-wait contract.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHIVulkan/RHIVulkanModule.hpp>
#include <Tests/UnitTest.hpp>

#include <limits>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanQueue, IdleAndTimelinePollsFollowV2Contract )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( {} );
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<RHI::IRHIQueue> Queue = *( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );

        LUMEN_CHECK_EQ( Queue->GetCompletedValue().Value, 0ULL );
        LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
        LUMEN_CHECK_TRUE( Queue->Wait( {}, 0.0 ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->Wait( { 0ULL }, 1.0 ).HasValue() );

        const RHI::FRHIExpected Pending = Queue->Wait( { 1ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( Pending );
        LUMEN_CHECK_EQ( Pending.Error().Code, RHI::ERHIErrorCode::Timeout );
        LUMEN_CHECK_EQ( Pending.Error().Condition, RHI::ERHIErrorCondition::Synchronization );

        LUMEN_CHECK_HAS_ERROR( Queue->Wait( { 1ULL }, -1.0 ) );
        LUMEN_CHECK_HAS_ERROR( Queue->Wait( { 1ULL }, std::numeric_limits<Float64>::quiet_NaN() ) );
        LUMEN_CHECK_HAS_ERROR( Queue->Wait( { 1ULL }, std::numeric_limits<Float64>::infinity() ) );

        const RHI::FRHIExpected InvalidSubmission = Queue->Submit( {} );
        LUMEN_CHECK_HAS_ERROR( InvalidSubmission );
        LUMEN_CHECK_EQ( InvalidSubmission.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( InvalidSubmission.Error().Condition, RHI::ERHIErrorCondition::Queue );
    }

} // namespace RHIVulkan

} // namespace Lumen
