/**
 * @file LaunchSurfaceBridgeTests.cpp
 * @brief Functional coverage for native-handle conversion and honest OpenGL bridge errors.
 */

#include <Engine/Modules/LaunchSurfaceBridge.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    /**
     * Private
     */

    inline FNativeWindowHandles MakeHandles ( const ENativeWindowSystem InWindowSystem ) noexcept
    {
        FNativeWindowHandles Handles;
        Handles.WindowSystem           = InWindowSystem;
        Handles.NativeWindow           = 101U;
        Handles.NativeDisplay          = 202U;
        Handles.NativeWindowAttachment = InWindowSystem == ENativeWindowSystem::Wayland ? 303U : 0U;
        Handles.Width                  = 320U;
        Handles.Height                 = 200U;
        return Handles;
    }

    /**
     * Public
     */

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescX11CopiesHandlesAndExtent )
    {
        const RHI::FRHIExtent3D Extent{ 320U, 200U, 1U };
        const auto Desc = Launch::MakeOpenGLNativeWindowDesc( MakeHandles( ENativeWindowSystem::X11 ), Extent );

        LUMEN_CHECK_HAS_VALUE( Desc );
        LUMEN_CHECK_EQ( Desc->WindowSystem, RHIOpenGL::EOpenGLNativeWindowSystem::X11 );
        LUMEN_CHECK_EQ( Desc->NativeWindow, 101U );
        LUMEN_CHECK_EQ( Desc->NativeDisplay, 202U );
        LUMEN_CHECK_EQ( Desc->NativeWindowAttachment, 0U );
        LUMEN_CHECK_EQ( Desc->Extent.Width, 320U );
        LUMEN_CHECK_EQ( Desc->Extent.Height, 200U );
        LUMEN_CHECK_EQ( Desc->Extent.Depth, 1U );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescWaylandCopiesAttachment )
    {
        const RHI::FRHIExtent3D Extent{ 640U, 480U, 1U };
        const auto Desc = Launch::MakeOpenGLNativeWindowDesc( MakeHandles( ENativeWindowSystem::Wayland ), Extent );

        LUMEN_CHECK_HAS_VALUE( Desc );
        LUMEN_CHECK_EQ( Desc->WindowSystem, RHIOpenGL::EOpenGLNativeWindowSystem::Wayland );
        LUMEN_CHECK_EQ( Desc->NativeWindow, 101U );
        LUMEN_CHECK_EQ( Desc->NativeDisplay, 202U );
        LUMEN_CHECK_EQ( Desc->NativeWindowAttachment, 303U );
        LUMEN_CHECK_EQ( Desc->Extent.Width, 640U );
        LUMEN_CHECK_EQ( Desc->Extent.Height, 480U );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescWaylandWithoutAttachment )
    {
        FNativeWindowHandles Handles   = MakeHandles( ENativeWindowSystem::Wayland );
        Handles.NativeWindowAttachment = 0U;

        const auto Desc = Launch::MakeOpenGLNativeWindowDesc( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } );

        LUMEN_CHECK_HAS_VALUE( Desc );
        LUMEN_CHECK_EQ( Desc->WindowSystem, RHIOpenGL::EOpenGLNativeWindowSystem::Wayland );
        LUMEN_CHECK_EQ( Desc->NativeWindowAttachment, 0U );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescUnknownReturnsNoValue )
    {
        const auto Desc = Launch::MakeOpenGLNativeWindowDesc( MakeHandles( ENativeWindowSystem::Unknown ),
                                                              RHI::FRHIExtent3D{ 320U, 200U, 1U } );

        LUMEN_CHECK_FALSE( Desc.HasValue() );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescZeroHandles )
    {
        for ( const ENativeWindowSystem WindowSystem : { ENativeWindowSystem::X11, ENativeWindowSystem::Wayland } )
        {
            FNativeWindowHandles Handles = MakeHandles( WindowSystem );
            Handles.NativeWindow         = 0U;
            LUMEN_CHECK_FALSE( Launch::MakeOpenGLNativeWindowDesc( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } ).HasValue() );

            Handles               = MakeHandles( WindowSystem );
            Handles.NativeDisplay = 0U;
            LUMEN_CHECK_FALSE( Launch::MakeOpenGLNativeWindowDesc( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } ).HasValue() );
        }
    }

    LUMEN_TEST( FLaunchSurfaceBridge, MakeOpenGLNativeWindowDescInvalidExtent )
    {
        const auto Desc =
            Launch::MakeOpenGLNativeWindowDesc( MakeHandles( ENativeWindowSystem::X11 ), RHI::FRHIExtent3D{ 0U, 0U, 0U } );

        LUMEN_CHECK_HAS_VALUE( Desc );
        LUMEN_CHECK_FALSE( Desc->Extent.IsValid() );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeX11CreatesBridge )
    {
        const auto Bridge =
            Launch::CreateSurfaceBridge( MakeHandles( ENativeWindowSystem::X11 ), RHI::FRHIExtent3D{ 320U, 200U, 1U } );

        LUMEN_CHECK_HAS_VALUE( Bridge );
        const auto Extent = Bridge->Get()->GetExtent();
        LUMEN_CHECK_HAS_VALUE( Extent );
        LUMEN_CHECK_EQ( Extent->Width, 320U );
        LUMEN_CHECK_EQ( Extent->Height, 200U );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeWaylandCreatesBridge )
    {
        const auto Bridge =
            Launch::CreateSurfaceBridge( MakeHandles( ENativeWindowSystem::Wayland ), RHI::FRHIExtent3D{ 640U, 480U, 1U } );

        LUMEN_CHECK_HAS_VALUE( Bridge );
        const auto Extent = Bridge->Get()->GetExtent();
        LUMEN_CHECK_HAS_VALUE( Extent );
        LUMEN_CHECK_EQ( Extent->Width, 640U );
        LUMEN_CHECK_EQ( Extent->Height, 480U );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeWaylandWithoutAttachment )
    {
        FNativeWindowHandles Handles   = MakeHandles( ENativeWindowSystem::Wayland );
        Handles.NativeWindowAttachment = 0U;

        const auto Bridge = Launch::CreateSurfaceBridge( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } );

        LUMEN_CHECK_HAS_ERROR( Bridge );
        LUMEN_CHECK_EQ( Bridge.Error().Code, RHI::ERHIErrorCode::Unsupported );
        LUMEN_CHECK_EQ( Bridge.Error().Condition, RHI::ERHIErrorCondition::Presentation );
        LUMEN_CHECK_TRUE( Bridge.Error().Message.contains( "EGL window attachment" ) );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeUnknownReportsInvalidPresentation )
    {
        const auto Bridge =
            Launch::CreateSurfaceBridge( MakeHandles( ENativeWindowSystem::Unknown ), RHI::FRHIExtent3D{ 320U, 200U, 1U } );

        LUMEN_CHECK_HAS_ERROR( Bridge );
        LUMEN_CHECK_EQ( Bridge.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Bridge.Error().Condition, RHI::ERHIErrorCondition::Presentation );
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeZeroHandles )
    {
        for ( const ENativeWindowSystem WindowSystem : { ENativeWindowSystem::X11, ENativeWindowSystem::Wayland } )
        {
            FNativeWindowHandles Handles = MakeHandles( WindowSystem );
            Handles.NativeWindow         = 0U;
            const auto MissingWindow     = Launch::CreateSurfaceBridge( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } );
            LUMEN_CHECK_HAS_ERROR( MissingWindow );
            LUMEN_CHECK_EQ( MissingWindow.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( MissingWindow.Error().Condition, RHI::ERHIErrorCondition::Presentation );

            Handles                   = MakeHandles( WindowSystem );
            Handles.NativeDisplay     = 0U;
            const auto MissingDisplay = Launch::CreateSurfaceBridge( Handles, RHI::FRHIExtent3D{ 320U, 200U, 1U } );
            LUMEN_CHECK_HAS_ERROR( MissingDisplay );
            LUMEN_CHECK_EQ( MissingDisplay.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( MissingDisplay.Error().Condition, RHI::ERHIErrorCondition::Presentation );
        }
    }

    LUMEN_TEST( FLaunchSurfaceBridge, CreateSurfaceBridgeInvalidExtent )
    {
        const auto Bridge =
            Launch::CreateSurfaceBridge( MakeHandles( ENativeWindowSystem::X11 ), RHI::FRHIExtent3D{ 0U, 0U, 0U } );

        LUMEN_CHECK_HAS_ERROR( Bridge );
        LUMEN_CHECK_EQ( Bridge.Error().Code, RHI::ERHIErrorCode::InvalidArgument );
        LUMEN_CHECK_EQ( Bridge.Error().Condition, RHI::ERHIErrorCondition::Presentation );
    }

} // namespace

} // namespace Lumen
