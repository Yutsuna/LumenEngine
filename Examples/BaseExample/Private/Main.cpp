#include "Generic/GenericApplication.hpp"
#include "Generic/GenericWindowDescription.hpp"
#include "Linux/LinuxApplication.hpp"
#include "Linux/LinuxWindow.hpp"
#include "Maths/Vector.hpp"

namespace LumenEngine
{

namespace
{

    static inline const FGenericWindowDescription GetWindowDescription ()
    {
        return ( const FGenericWindowDescription ){
            .Title        = "Lumen Engine - Linux Window",
            .Position     = Math::FVec2i( 100, 100 ),
            .Size         = Math::FVec2i( 1280, 720 ),
            .WindowMode   = EWindowMode::Windowed,
            .bIsResizable = true,
            .bIsVisible   = true };
    }

} // namespace

} // namespace LumenEngine

int main ( void )
{
    auto App          = LumenEngine::FLinuxApplication::CreateLinuxApplication();
    auto WindowDesc   = LumenEngine::MakeShared<LumenEngine::FGenericWindowDescription>( LumenEngine::GetWindowDescription() );
    auto Window       = App->MakeWindow();
    auto ParentWindow = nullptr;

    App->InitializeWindow( Window, WindowDesc, ParentWindow, true );

    return 0;
}
