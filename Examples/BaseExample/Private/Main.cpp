#include "CoreTypes.hpp"
#include "LaunchEngine.hpp"
#include "MessageHandler.hpp"

#include "Generic/GenericApplication.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

#include "ErrorCodes.hpp"

LumenEngine::Int32 LumenEngine::Launch::ClientInit ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED **Argv )
{
    FLogCategory LogBaseExample( "BaseExample" );

    LUMEN_LOG_INFO( LogBaseExample, "Client initialization started." );

    if ( !GPlatformApplication.IsValid() )
    {
        LUMEN_LOG_FATAL( LogBaseExample, "Platform Application is not valid during ClientInit." );
        return EErrorCode::Type::Failure;
    }

    GPlatformApplication->SetMessageHandler( MakeShared<FBaseExampleMessageHandler>() );

    LUMEN_LOG_INFO( LogBaseExample, "Client initialization completed successfully." );
    return EErrorCode::Type::Success;
}

void LumenEngine::Launch::ClientTick ( const Float64 LUMEN_UNUSED DeltaTime )
{
    //
}
