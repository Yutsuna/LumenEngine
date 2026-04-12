#include "Application.hpp"

#include "Container/Inline/UniquePtr.inl"
#include "ErrorCodes.hpp"
#include "LaunchEngine.hpp"

namespace LumenEngine
{

namespace
{
    const TUniquePtr<FTriangleExampleApplication> GAppInstance = MakeUnique<FTriangleExampleApplication>();
}

Int32 Launch::ClientInit ( const Int32 LUMEN_UNUSED Argc, const AnsiChar LUMEN_UNUSED **Argv )
{
    if ( not GAppInstance.IsValid() )
    {
        return EErrorCode::Type::Failure;
    }

    return GAppInstance->Initialize();
}

void Launch::ClientTick ( const Float64 DeltaTime )
{
    if ( GAppInstance.IsValid() )
    {
        GAppInstance->Tick( DeltaTime );
    }
}

} // namespace LumenEngine
