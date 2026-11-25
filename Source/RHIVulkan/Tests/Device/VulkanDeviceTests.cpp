/**
 * @file VulkanDeviceTests.cpp
 * @brief Integration tests for Vulkan device creation, queue lookup, and loss propagation.
 */

#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Validation/RHIValidation.hpp>
#include <RHIVulkan/Device/VulkanDevice.hpp>
#include <RHIVulkan/RHIVulkanModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanDevice, PublishesContractAndLooksUpGraphicsQueue )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );
        LUMEN_CHECK_FALSE( Adapters->empty() );

        RHI::FRHIDeviceDesc Desc;
        Desc.AdapterId.Emplace( ( *Adapters )[0].AdapterId );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( Desc );
        LUMEN_CHECK_TRUE( Device.HasValue() );
        LUMEN_CHECK_EQ( ( *Device )->GetStatus(), RHI::ERHIDeviceStatus::Ready );
        LUMEN_CHECK_TRUE( ( *Device )->GetId().IsValid() );
        LUMEN_CHECK_TRUE( ( *Device )->GetCapabilities().Validate().HasValue() );
        LUMEN_CHECK_TRUE( ( *Device )->GetLimits().Validate().HasValue() );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Queue =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_TRUE( Queue.HasValue() );
        LUMEN_CHECK_EQ( ( *Queue )->GetType(), RHI::ERHIQueueType::Graphics );
        LUMEN_CHECK_TRUE( ( *Queue )->GetId().IsValid() );
        LUMEN_CHECK_EQ( ( *Queue )->GetCompletedValue().Value, 0ULL );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Missing =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 1U );
        LUMEN_CHECK_HAS_ERROR( Missing );
        LUMEN_CHECK_EQ( Missing.Error().Code, RHI::ERHIErrorCode::NotFound );
        LUMEN_CHECK_EQ( Missing.Error().Condition, RHI::ERHIErrorCondition::Queue );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Invalid =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Count, 0U );
        LUMEN_CHECK_HAS_ERROR( Invalid );
        LUMEN_CHECK_EQ( Invalid.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Invalid.Error().Condition, RHI::ERHIErrorCondition::Queue );
    }

    LUMEN_TEST( VulkanDevice, CreatesEveryAdvertisedDedicatedQueueClass )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TVector<RHI::FRHIAdapterInfo>, RHI::FRHIError> Adapters = ( *Module )->EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );
        LUMEN_CHECK_FALSE( Adapters->empty() );

        const RHI::FRHIAdapterInfo &Adapter = ( *Adapters )[0];
        RHI::FRHIDeviceDesc Desc;
        Desc.AdapterId.Emplace( Adapter.AdapterId );
        Desc.Queues.clear();
        for ( const RHI::FRHIQueueFamilyInfo &Family : Adapter.QueueFamilies )
        {
            Desc.Queues.push_back( { .Type = Family.Type, .Count = 1U } );
        }
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( Desc );
        LUMEN_CHECK_TRUE( Device.HasValue() );

        for ( const RHI::FRHIQueueFamilyInfo &Family : Adapter.QueueFamilies )
        {
            const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Queue = ( *Device )->GetQueue( Family.Type, 0U );
            LUMEN_CHECK_TRUE( Queue.HasValue() );
            LUMEN_CHECK_EQ( ( *Queue )->GetType(), Family.Type );
            LUMEN_CHECK_TRUE( ( *Queue )->WaitIdle().HasValue() );
        }
    }

    LUMEN_TEST( VulkanDevice, LossPropagatesToQueueAndFactoryOperations )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIVulkanModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( {} );
        LUMEN_CHECK_TRUE( Device.HasValue() );
        TSharedRef<RHI::IRHIQueue> Queue = *( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );

        const TOptional<FVulkanDevice *> NativeDevice = RHI::TryUnwrapRHIBackend<FVulkanDevice>( ( *Device ).Get() );
        LUMEN_CHECK_TRUE( NativeDevice.HasValue() );
        ( *NativeDevice )->InjectDeviceLost();
        LUMEN_CHECK_EQ( ( *Device )->GetStatus(), RHI::ERHIDeviceStatus::Lost );

        const RHI::FRHIExpected DeviceIdle = ( *Device )->WaitIdle();
        LUMEN_CHECK_HAS_ERROR( DeviceIdle );
        LUMEN_CHECK_EQ( DeviceIdle.Error().Code, RHI::ERHIErrorCode::DeviceLost );
        const RHI::FRHIExpected QueueIdle = Queue->WaitIdle();
        LUMEN_CHECK_HAS_ERROR( QueueIdle );
        LUMEN_CHECK_EQ( QueueIdle.Error().Code, RHI::ERHIErrorCode::DeviceLost );
        const RHI::FRHIExpected QueueWait = Queue->Wait( { 1ULL }, 0.0 );
        LUMEN_CHECK_HAS_ERROR( QueueWait );
        LUMEN_CHECK_EQ( QueueWait.Error().Code, RHI::ERHIErrorCode::DeviceLost );

        const TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Buffer =
            ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource } );
        LUMEN_CHECK_HAS_ERROR( Buffer );
        LUMEN_CHECK_EQ( Buffer.Error().Code, RHI::ERHIErrorCode::DeviceLost );
        LUMEN_CHECK_EQ( Buffer.Error().Condition, RHI::ERHIErrorCondition::Device );
    }

} // namespace RHIVulkan

} // namespace Lumen
