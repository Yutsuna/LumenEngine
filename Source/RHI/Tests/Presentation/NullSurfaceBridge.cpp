/**
 * @file NullSurfaceBridge.cpp
 * @brief Implements the deterministic test native surface bridge.
 */

#include <Presentation/NullSurfaceBridge.hpp>

Lumen::RHI::Tests::FNullSurfaceBridge::FNullSurfaceBridge ( const FRHIExtent3D InExtent ) noexcept : Extent( InExtent )
{
}

Lumen::TExpected<Lumen::RHI::FRHINativeSurface, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullSurfaceBridge::CreateNativeSurface ( const UPtr InBackendHandle ) noexcept
{
    ++CreateCount;
    return FRHINativeSurface{ .BackendHandle = InBackendHandle, .SurfaceHandle = 1U, .ContextHandle = 2U };
}

void Lumen::RHI::Tests::FNullSurfaceBridge::DestroyNativeSurface ( const FRHINativeSurface &InSurface ) noexcept
{
    if ( InSurface.SurfaceHandle != 0U )
    {
        ++DestroyCount;
    }
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullSurfaceBridge::MakeCurrent ( const FRHINativeSurface &InSurface ) noexcept
{
    if ( InSurface.SurfaceHandle == 0U or InSurface.ContextHandle == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Null surface or context is missing" ) );
    }
    return {};
}

Lumen::RHI::FRHIExpected Lumen::RHI::Tests::FNullSurfaceBridge::Present ( const FRHINativeSurface &InSurface,
                                                                          const Int32 InSwapInterval ) noexcept
{
    if ( InSurface.SurfaceHandle == 0U or InSwapInterval < 0 )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidPresentation( "Invalid Null surface presentation request" ) );
    }
    return {};
}

Lumen::TExpected<Lumen::RHI::FRHIExtent3D, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullSurfaceBridge::GetExtent () const noexcept
{
    return Extent;
}

Lumen::TOptional<Lumen::UPtr> Lumen::RHI::Tests::FNullSurfaceBridge::ResolveProcedure ( const FStringView InName ) noexcept
{
    if ( InName == "NullProcedure" )
    {
        return TOptional<UPtr>( 1U );
    }
    return {};
}

Lumen::UInt32 Lumen::RHI::Tests::FNullSurfaceBridge::GetCreateCount () const noexcept
{
    return CreateCount;
}

Lumen::UInt32 Lumen::RHI::Tests::FNullSurfaceBridge::GetDestroyCount () const noexcept
{
    return DestroyCount;
}
