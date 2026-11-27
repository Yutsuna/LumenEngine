/**
 * @file OpenGLModuleTests.cpp
 * @brief Functional tests for the OpenGL RHI module entry point.
 */

#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>
#include <RHIOpenGL/RHIOpenGLModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    LUMEN_TEST( OpenGLBootstrap, CreateReturnsModule )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
    }

    LUMEN_TEST( OpenGLBootstrap, ApiVersionIsOneOneZero )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );
        const RHI::FRHIApiVersion Version = ( *Module )->GetApiVersion();
        LUMEN_CHECK_EQ( Version.Major, 1U );
        LUMEN_CHECK_EQ( Version.Minor, 1U );
        LUMEN_CHECK_EQ( Version.Patch, 0U );
    }

    LUMEN_TEST( OpenGLModule, CreateDeviceRejectsInvalidDescriptor )
    {
        const TExpected<TSharedRef<RHI::IRHIModule>, RHI::FRHIError> Module = CreateRHIOpenGLModule();
        LUMEN_CHECK_TRUE( Module.HasValue() );

        RHI::FRHIDeviceDesc InvalidDesc;
        InvalidDesc.Queues.clear();
        const TExpected<TSharedRef<RHI::IRHIDevice>, RHI::FRHIError> Device = ( *Module )->CreateDevice( InvalidDesc );
        LUMEN_CHECK_HAS_ERROR( Device );
        LUMEN_CHECK_EQ( Device.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
    }

} // namespace RHIOpenGL

} // namespace Lumen
