/**
 * @file GenericApplicationMessageHandlerTests.cpp
 * @brief Functional coverage for SDL input and window event translation.
 */

#include <Generic/GenericApplicationMessageHandler.hpp>
#include <Generic/GenericWindow.hpp>
#include <Linux/LinuxApplication.hpp>
#include <Linux/LinuxWindow.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    class FRecordingMessageHandler final : public FGenericApplicationMessageHandler
    {
    public:

        void OnMouseMove ( const Int32 InX, const Int32 InY ) noexcept override
        {
            MouseX = InX;
            MouseY = InY;
        }

        void OnMouseWheel ( const Int32 InDelta ) noexcept override
        {
            WheelDelta = InDelta;
        }

        void OnMouseDoubleClick ( const EMouseButton::Type InMouseButton ) noexcept override
        {
            DoubleClickButton = InMouseButton;
            ++DoubleClickCount;
        }

        void OnWindowResized ( const TSharedRef<FGenericWindow> &InWindow ) noexcept override
        {
            ResizedSize = InWindow->GetWindowSize();
            ++ResizeCount;
        }

        Int32 MouseX                         = 0;
        Int32 MouseY                         = 0;
        Int32 WheelDelta                     = 0;
        Int32 DoubleClickCount               = 0;
        Int32 ResizeCount                    = 0;
        EMouseButton::Type DoubleClickButton = EMouseButton::Invalid;
        Maths::FVec2i ResizedSize            = Maths::FVec2i( 0, 0 );
    };

    LUMEN_TEST( GenericWindowDescription, DefaultsToNoGraphicsApi )
    {
        const FGenericWindowDescription Description;
        LUMEN_CHECK_EQ( Description.GraphicsApi, EWindowGraphicsApi::None );
    }

#if defined( LUMEN_PLATFORM_LINUX )
    LUMEN_TEST( LinuxApplicationEvents, RoutesMouseAndPixelResize )
    {
        if ( not FLinuxBackend::InitializeSDL() )
        {
            return;
        }

        FLinuxApplication Application;
        const TSharedRef<FRecordingMessageHandler> Handler = MakeShared<FRecordingMessageHandler>();
        Application.SetMessageHandler( Handler );

        const TSharedRef<FGenericWindowDescription> Description = MakeShared<FGenericWindowDescription>();
        Description->bIsVisible                                 = false;
        Description->GraphicsApi                                = EWindowGraphicsApi::OpenGL;
        const TSharedRef<FGenericWindow> Window                 = Application.MakeWindow();
        Application.InitializeWindow( Window, Description, nullptr, false );
        if ( StaticCastSharedRef<FLinuxWindow>( Window )->GetOSWindowHandle() == nullptr )
        {
            Application.Shutdown();
            FLinuxBackend::ShutdownSDL();
            return;
        }

        SDL_Event Event{};
        Event.type            = SDL_EVENT_MOUSE_MOTION;
        Event.motion.windowID = StaticCastSharedRef<FLinuxWindow>( Window )->GetOSWindowID();
        Event.motion.x        = 17.0F;
        Event.motion.y        = 29.0F;
        Application.AddPendingEvent( Event );
        LUMEN_CHECK_EQ( Handler->MouseX, 17 );
        LUMEN_CHECK_EQ( Handler->MouseY, 29 );

        Event                 = SDL_Event{};
        Event.type            = SDL_EVENT_MOUSE_BUTTON_DOWN;
        Event.button.windowID = StaticCastSharedRef<FLinuxWindow>( Window )->GetOSWindowID();
        Event.button.button   = SDL_BUTTON_LEFT;
        Event.button.clicks   = 2U;
        Application.AddPendingEvent( Event );
        LUMEN_CHECK_EQ( Handler->DoubleClickCount, 1 );
        LUMEN_CHECK_EQ( Handler->DoubleClickButton, EMouseButton::Left );

        Event                = SDL_Event{};
        Event.type           = SDL_EVENT_MOUSE_WHEEL;
        Event.wheel.windowID = StaticCastSharedRef<FLinuxWindow>( Window )->GetOSWindowID();
        Event.wheel.y        = -1.0F;
        Application.AddPendingEvent( Event );
        LUMEN_CHECK_EQ( Handler->WheelDelta, -100 );

        Event                 = SDL_Event{};
        Event.type            = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
        Event.window.windowID = StaticCastSharedRef<FLinuxWindow>( Window )->GetOSWindowID();
        Application.AddPendingEvent( Event );
        LUMEN_CHECK_EQ( Handler->ResizeCount, 1 );
        LUMEN_CHECK_EQ( Handler->ResizedSize.X, Description->Size.X );
        LUMEN_CHECK_EQ( Handler->ResizedSize.Y, Description->Size.Y );

        Application.Shutdown();
        FLinuxBackend::ShutdownSDL();
    }
#endif

} // namespace

} // namespace Lumen
