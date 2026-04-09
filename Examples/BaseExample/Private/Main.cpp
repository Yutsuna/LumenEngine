#include "Application.hpp"

#include "Container/Inline/UniquePtr.inl"
#include "ErrorCodes.hpp"
#include "LaunchEngine.hpp"

namespace
{
LumenEngine::TUniquePtr<LumenEngine::FBaseApplication> GAppInstance;
}

LumenEngine::Int32 LumenEngine::Launch::ClientInit ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED **Argv )
{
    GAppInstance = MakeUnique<FBaseApplication>();

    if ( not GAppInstance.IsValid() )
    {
        return EErrorCode::Type::Failure;
    }

    return GAppInstance->Initialize();
}

void LumenEngine::Launch::ClientTick ( const Float64 DeltaTime )
{
    if ( GAppInstance.IsValid() )
    {
        GAppInstance->Tick( DeltaTime );
    }
}
