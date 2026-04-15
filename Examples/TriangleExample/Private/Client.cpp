#include "Application.hpp"

#include "Container/Inline/UniquePtr.inl"
#include "ErrorCodes.hpp"
#include "LaunchEngine.hpp"

namespace LumenEngine
{

namespace
{
    TUniquePtr<FTriangleExampleApplication> GAppInstance = nullptr;
}

Int32 Launch::ClientInit ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED **Argv )
{
    GAppInstance = MakeUnique<FTriangleExampleApplication>();
    if ( not GAppInstance.IsValid() )
    {
        return EErrorCode::Failure;
    }

    return GAppInstance->Initialize();
}

void Launch::ClientTick ( const Float64 DeltaTime )
{
    GAppInstance->Tick( DeltaTime );
}

void Launch::ClientShutdown ()
{
    GAppInstance.Reset();
}

} // namespace LumenEngine
