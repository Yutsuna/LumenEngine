/**
 * @file OpenGLSurfaceBridgeFactoryTests.cpp
 * @brief Functional coverage for native surface factory validation and explicit capability errors.
 */

#include <RHIOpenGL/RHIOpenGLSurfaceBridge.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    using namespace RHIOpenGL;

    [[nodiscard]] FOpenGLNativeWindowDesc MakeNativeWindowDesc ( const EOpenGLNativeWindowSystem InWindowSystem ) noexcept
    {
        FOpenGLNativeWindowDesc Desc;
        Desc.WindowSystem           = InWindowSystem;
        Desc.NativeWindow           = 7U;
        Desc.NativeDisplay          = 8U;
        Desc.NativeWindowAttachment = InWindowSystem == EOpenGLNativeWindowSystem::Wayland ? 9U : 0U;
        Desc.Extent                 = { 320U, 200U, 1U };
        return Desc;
    }

    LUMEN_TEST( OpenGLSurfaceBridgeFactory, RejectsIncoherentNativeHandles )
    {
        const TExpected<TSharedRef<RHI::IRHISurfaceBridge>, RHI::FRHIError> Bridge = CreateRHIOpenGLSurfaceBridge( {} );
        LUMEN_CHECK_HAS_ERROR( Bridge );
        LUMEN_CHECK_EQ( Bridge.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Bridge.Error().Condition, RHI::ERHIErrorCondition::Presentation );
    }

    LUMEN_TEST( OpenGLSurfaceBridgeFactory, WaylandRequiresNativeEglWindowAttachment )
    {
        FOpenGLNativeWindowDesc Desc = MakeNativeWindowDesc( EOpenGLNativeWindowSystem::Wayland );
        Desc.NativeWindowAttachment  = 0U;
        const TExpected<TSharedRef<RHI::IRHISurfaceBridge>, RHI::FRHIError> Bridge = CreateRHIOpenGLSurfaceBridge( Desc );
        LUMEN_CHECK_HAS_ERROR( Bridge );
        LUMEN_CHECK_EQ( Bridge.Error().Code, RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_TRUE( Bridge.Error().Message.contains( "EGL window attachment" ) );
    }

    LUMEN_TEST( OpenGLSurfaceBridgeFactory, CoherentWindowsCreateDetachedBridges )
    {
        for ( const EOpenGLNativeWindowSystem WindowSystem :
              { EOpenGLNativeWindowSystem::X11, EOpenGLNativeWindowSystem::Wayland } )
        {
            const TExpected<TSharedRef<RHI::IRHISurfaceBridge>, RHI::FRHIError> Bridge =
                CreateRHIOpenGLSurfaceBridge( MakeNativeWindowDesc( WindowSystem ) );
            LUMEN_CHECK_HAS_VALUE( Bridge );
            const auto Extent = Bridge->Get()->GetExtent();
            LUMEN_CHECK_HAS_VALUE( Extent );
            LUMEN_CHECK_EQ( Extent->Width, 320U );
            LUMEN_CHECK_EQ( Extent->Height, 200U );
        }
    }

} // namespace

} // namespace Lumen
