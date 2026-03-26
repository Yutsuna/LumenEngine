#include "MessageHandler.hpp"

#include "Generic/GenericApplication.hpp"

#include "Logging/Logger.hpp"
#include "Logging/LoggingCategory.hpp"

#include "ErrorCodes.hpp"

namespace LumenEngine
{

namespace Launch
{

    Int32 ClientInit ( const Int32, const AnsiChar *[] )
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

} // namespace Launch

} // namespace LumenEngine
