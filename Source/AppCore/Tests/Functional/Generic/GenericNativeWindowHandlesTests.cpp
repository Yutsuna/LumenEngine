/**
 * @file GenericNativeWindowHandlesTests.cpp
 * @brief Functional coverage for portable native-window handle values.
 */

#include <Generic/GenericWindow.hpp>
#include <Linux/LinuxWindow.hpp>

#include <Tests/UnitTest.hpp>

#include <type_traits>

namespace Lumen
{

namespace
{

    static_assert( std::is_trivially_copyable_v<FNativeWindowHandles> );

    LUMEN_TEST( GenericNativeWindowHandles, DefaultValueIsEmpty )
    {
        const FNativeWindowHandles Handles;
        LUMEN_CHECK_EQ( Handles.WindowSystem, ENativeWindowSystem::Unknown );
        LUMEN_CHECK_EQ( Handles.NativeWindow, 0U );
        LUMEN_CHECK_EQ( Handles.NativeDisplay, 0U );
        LUMEN_CHECK_EQ( Handles.NativeWindowAttachment, 0U );
        LUMEN_CHECK_EQ( Handles.Width, 0U );
        LUMEN_CHECK_EQ( Handles.Height, 0U );
    }

    LUMEN_TEST( GenericNativeWindowHandles, GenericWindowHasNoNativeHandleProvider )
    {
        const FGenericWindow Window;
        LUMEN_CHECK_FALSE( Window.GetNativeHandles().HasValue() );
    }

#if defined( LUMEN_PLATFORM_LINUX )
    LUMEN_TEST( GenericNativeWindowHandles, UninitializedLinuxWindowHasNoNativeHandles )
    {
        const TSharedRef<FLinuxWindow> Window = FLinuxWindow::Make();
        LUMEN_CHECK_FALSE( Window->GetNativeHandles().HasValue() );
    }
#endif

} // namespace

} // namespace Lumen
