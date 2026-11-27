/**
 * @file Application.cpp
 * @brief Implementation of the FLuminaApplication class.
 */

#include <Application.hpp>

#include <MessageHandler.hpp>

namespace Lumen
{

/**
 * Registration
 */

LUMEN_REGISTER_MODULE( FLuminaApplication )
const FLogCategory LogLumina{ "Lumina" };

/**
 * Public
 */

FLuminaApplication::FLuminaApplication ( const Launch::TModuleContext<FLuminaApplication> &InContext ) noexcept
{
    if ( Launch::FLaunchApplicationModule *const ApplicationModule = InContext.Find<Launch::FLaunchApplicationModule>() )
    {
        MessageHandlerGuard = ApplicationModule->PushMessageHandler<FLuminaMessageHandler>();
    }

    LUMEN_LOG_VERY_VERBOSE( LogLumina, "FLuminaApplication constructed" );
}

} // namespace Lumen
