/**
 * @file OpenGLDeviceTests.cpp
 * @brief Integration tests for OpenGL device creation, queues, and loss.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Validation/RHIValidation.hpp>
#include <RHIOpenGL/Device/OpenGLDevice.hpp>
#include <RHIOpenGL/RHIOpenGLModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    LUMEN_TEST( OpenGLDevice, PublishesContractAndLooksUpGraphicsQueue )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( RHI::FRHIDeviceDesc{} );
        LUMEN_CHECK_TRUE( Device.HasValue() );
        LUMEN_CHECK_EQ( ( *Device )->GetStatus(), RHI::ERHIDeviceStatus::Ready );
        LUMEN_CHECK_TRUE( ( *Device )->GetLimits().Validate().HasValue() );
        LUMEN_CHECK_TRUE( ( *Device )->GetCapabilities().Validate().HasValue() );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Graphics =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 0U );
        LUMEN_CHECK_TRUE( Graphics.HasValue() );
        LUMEN_CHECK_EQ( ( *Graphics )->GetType(), RHI::ERHIQueueType::Graphics );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Missing =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Graphics, 1U );
        LUMEN_CHECK_HAS_ERROR( Missing );
        LUMEN_CHECK_EQ( Missing.Error().Code, RHI::ERHIErrorCode::NotFound );

        const TExpected<TSharedRef<RHI::IRHIQueue>, RHI::FRHIError> Invalid =
            ( *Device )->GetQueue( RHI::ERHIQueueType::Count, 0U );
        LUMEN_CHECK_HAS_ERROR( Invalid );
        LUMEN_CHECK_EQ( Invalid.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
    }

    LUMEN_TEST( OpenGLDevice, LossPropagatesToQueueAndFactoryOperations )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( RHI::FRHIDeviceDesc{} );
        LUMEN_CHECK_TRUE( Device.HasValue() );

        const TOptional<FOpenGLDevice *> Native = RHI::TryUnwrapRHIBackend<FOpenGLDevice>( ( *Device ).Get() );
        LUMEN_CHECK_TRUE( Native.HasValue() );
        ( *Native )->InjectDeviceLost();
        LUMEN_CHECK_EQ( ( *Device )->GetStatus(), RHI::ERHIDeviceStatus::Lost );

        const RHI::FRHIExpected Idle = ( *Device )->WaitIdle();
        LUMEN_CHECK_HAS_ERROR( Idle );
        LUMEN_CHECK_EQ( Idle.Error().Code, RHI::ERHIErrorCode::DeviceLost );

        const TExpected<TSharedRef<RHI::IRHIBuffer>, RHI::FRHIError> Buffer =
            ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = RHI::ERHIResourceUsage::CopySource } );
        LUMEN_CHECK_HAS_ERROR( Buffer );
        LUMEN_CHECK_EQ( Buffer.Error().Code, RHI::ERHIErrorCode::DeviceLost );
    }

} // namespace RHIOpenGL

} // namespace Lumen
