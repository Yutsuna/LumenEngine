/**
 * @file RHISurfaceDescTests.cpp
 * @brief Tests backend-neutral surface payload validation.
 */

#include <Presentation/NullSurfaceBridge.hpp>
#include <RHI/Presentation/RHISurfaceDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHISurfaceDesc, AcceptsTypedCompletePayloads )
        {
            FRHISurfaceDesc Desc;
            Desc.Type          = ERHISurfaceType::Null;
            Desc.NativeWindow  = 1U;
            Desc.InitialExtent = { 1280U, 720U, 1U };
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );

            Desc.Type             = ERHISurfaceType::Window;
            Desc.NativeDisplay    = 2U;
            Desc.bRequiresDisplay = true;
            Desc.Bridge           = TSharedPtr<IRHISurfaceBridge>( MakeShared<Tests::FNullSurfaceBridge>() );
            LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        }

        LUMEN_TEST( RHISurfaceDesc, RejectsMissingHandlesAndExtents )
        {
            LUMEN_CHECK_HAS_ERROR( FRHISurfaceDesc{}.Validate() );
            FRHISurfaceDesc Desc;
            Desc.Type             = ERHISurfaceType::Window;
            Desc.NativeWindow     = 1U;
            Desc.InitialExtent    = { 1U, 1U, 1U };
            Desc.bRequiresDisplay = true;
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
            Desc.Bridge        = TSharedPtr<IRHISurfaceBridge>( MakeShared<Tests::FNullSurfaceBridge>() );
            Desc.NativeDisplay = 2U;
            Desc.InitialExtent = { 0U, 1U, 1U };
            LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        }

        LUMEN_TEST( RHISurfaceBridge, ExposesNativeSurfaceAndContextOperations )
        {
            Tests::FNullSurfaceBridge Bridge( { 640U, 480U, 1U } );
            TExpected<FRHINativeSurface, FRHIError> Surface = Bridge.CreateNativeSurface( 7U );
            LUMEN_CHECK_TRUE( Surface.HasValue() );
            LUMEN_CHECK_EQ( Surface->BackendHandle, 7U );
            LUMEN_CHECK_EQ( Surface->SurfaceHandle, 1U );
            LUMEN_CHECK_EQ( Surface->ContextHandle, 2U );
            LUMEN_CHECK_TRUE( Bridge.MakeCurrent( *Surface ).HasValue() );
            LUMEN_CHECK_TRUE( Bridge.Present( *Surface, 1 ).HasValue() );
            TExpected<FRHIExtent3D, FRHIError> Extent = Bridge.GetExtent();
            LUMEN_CHECK_TRUE( Extent.HasValue() );
            LUMEN_CHECK_EQ( Extent->Width, 640U );
            TOptional<UPtr> Procedure = Bridge.ResolveProcedure( "NullProcedure" );
            LUMEN_CHECK_TRUE( Procedure.HasValue() );
            LUMEN_CHECK_EQ( *Procedure, 1U );
            LUMEN_CHECK_FALSE( Bridge.ResolveProcedure( "Missing" ).HasValue() );
            Bridge.DestroyNativeSurface( *Surface );
            LUMEN_CHECK_EQ( Bridge.GetCreateCount(), 1U );
            LUMEN_CHECK_EQ( Bridge.GetDestroyCount(), 1U );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
