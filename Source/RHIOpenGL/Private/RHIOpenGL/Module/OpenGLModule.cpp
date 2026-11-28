/**
 * @file OpenGLModule.cpp
 * @brief Implements OpenGL adapter discovery and device creation.
 */

#include <RHIOpenGL/Module/OpenGLModule.hpp>

#include <RHIOpenGL/Device/OpenGLCapabilityProbe.hpp>
#include <RHIOpenGL/Device/OpenGLDevice.hpp>
#include <RHIOpenGL/Loader/GLLoader.hpp>
#include <RHIOpenGL/Platform/Linux/EGLPlatformContext.hpp>

#include <RHI/Device/RHIBackendSupport.hpp>
#include <RHI/Module/RHIAdapterInfo.hpp>
#include <RHI/Types/RHIError.hpp>

#include <Algo/Find.hpp>

#include <Templates/Optional.hpp>

/**
 * File-local helpers
 */

namespace
{

[[nodiscard]] Lumen::TExpected<const Lumen::RHI::FRHIAdapterInfo *, Lumen::RHI::FRHIError>
SelectOpenGLAdapter ( const Lumen::TVector<Lumen::RHI::FRHIAdapterInfo> &InAdapters,
                      const Lumen::TOptional<Lumen::UInt64> &InAdapterId ) noexcept
{
    if ( InAdapters.empty() )
    {
        return Lumen::TUnexpected( Lumen::RHI::MakeRHIUnsupported( Lumen::RHI::ERHIErrorCondition::Device,
                                                                   "No OpenGL 4.6 adapter is available in this environment." ) );
    }
    if ( InAdapterId )
    {
        if ( const Lumen::RHI::FRHIAdapterInfo *Found = Lumen::Algo::FindIf(
                 InAdapters, [&InAdapterId] ( const Lumen::RHI::FRHIAdapterInfo &Adapter ) noexcept -> Lumen::Bool
        {
            return Adapter.AdapterId == *InAdapterId;
        } ) )
        {
            return Found;
        }
        return Lumen::TUnexpected(
            Lumen::RHI::MakeRHINotFound( Lumen::RHI::ERHIErrorCondition::Device, "Requested OpenGL AdapterId was not found." ) );
    }
    return &InAdapters.front();
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIModule>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLModule::Create ( const FOpenGLNativeWindowDesc *InNativeWindow ) noexcept
{
    if ( InNativeWindow != nullptr )
    {
        return MakeShared<FOpenGLModule>( *InNativeWindow );
    }
    return MakeShared<FOpenGLModule>();
}

Lumen::RHIOpenGL::FOpenGLModule::FOpenGLModule ( const FOpenGLNativeWindowDesc &InNativeWindow ) noexcept
    : NativeWindow( InNativeWindow )
{
    /* */
}

Lumen::RHI::FRHIApiVersion Lumen::RHIOpenGL::FOpenGLModule::GetApiVersion () const noexcept
{
    return { .Major = 1U, .Minor = 1U, .Patch = 0U };
}

Lumen::TExpected<Lumen::TVector<Lumen::RHI::FRHIAdapterInfo>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLModule::EnumerateAdapters () const noexcept
{
    FEGLPlatformContext Probe;
    LUMEN_TRY_EXPECT_ERROR( Probe.Initialize( false, NativeWindow ? &*NativeWindow : nullptr ) );

    FGLDispatchTable Dispatch;
    LUMEN_TRY_EXPECT_ERROR( LoadGLDispatchTable( Dispatch, Probe ) );

    RHI::FRHIAdapterInfo Info;
    LUMEN_TRY_EXPECT_ERROR( ProbeOpenGLCapabilities( Info, Dispatch ) );

    TVector<RHI::FRHIAdapterInfo> Result;
    Result.push_back( static_cast<RHI::FRHIAdapterInfo &&>( Info ) );
    return Result;
}

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHI::IRHIDevice>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLModule::CreateDevice ( const RHI::FRHIDeviceDesc &InDesc ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    LUMEN_EXPECT( Adapters, EnumerateAdapters() );
    LUMEN_EXPECT( Adapter, SelectOpenGLAdapter( Adapters, InDesc.AdapterId ) );
    LUMEN_EXPECT( CapabilityGate, RHI::CheckRHICapabilityGate( *Adapter, InDesc ) );

    for ( const RHI::FRHIQueueRequest &Request : InDesc.Queues )
    {
        const RHI::FRHIQueueFamilyInfo *Family =
            Algo::FindIf( Adapter->QueueFamilies, [&] ( const RHI::FRHIQueueFamilyInfo &Candidate )
        {
            return Candidate.Type == Request.Type;
        } );
        if ( Family == nullptr )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue,
                                                         "Requested queue type is not exposed by this OpenGL adapter." ) );
        }
        if ( Request.Count > Family->Count )
        {
            return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Queue,
                                                         "Requested OpenGL queue count exceeds the family count." ) );
        }
    }

    return FOpenGLDevice::Create( *Adapter, InDesc, NativeWindow ? &*NativeWindow : nullptr );
}
