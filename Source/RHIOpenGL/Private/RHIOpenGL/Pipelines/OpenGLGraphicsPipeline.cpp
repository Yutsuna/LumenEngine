/**
 * @file OpenGLGraphicsPipeline.cpp
 * @brief Implements GLSL program linking for graphics pipelines.
 */

#include <RHIOpenGL/Pipelines/OpenGLGraphicsPipeline.hpp>

#include <Algo/ForEach.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <Templates/Vector.hpp>

#include <RHIOpenGL/Pipelines/OpenGLPipelineLayout.hpp>
#include <RHIOpenGL/Shaders/OpenGLShader.hpp>

/**
 * File-local helpers
 */

namespace
{

constexpr Lumen::RHIOpenGL::GLenum GlUniformBlockInterface = 0x92E2U;
constexpr Lumen::RHIOpenGL::GLuint GlInvalidIndex          = 0xFFFFFFFFU;
constexpr Lumen::USize GlBlockNameCapacity                 = 48U;

/** @brief Builds a candidate block name from a reflection slot. */
void MakeBlockName ( Lumen::AnsiChar *OutName,
                     const Lumen::USize InCapacity,
                     const Lumen::AnsiChar *InPrefix,
                     const Lumen::UInt32 InSlot ) noexcept
{
    Lumen::USize Length = 0U;
    while ( InPrefix[Length] != '\0' and Length + 1U < InCapacity )
    {
        OutName[Length] = InPrefix[Length];
        ++Length;
    }
    Lumen::AnsiChar Digits[12] = {};
    Lumen::USize DigitCount    = 0U;
    Lumen::UInt32 Slot         = InSlot;
    do
    {
        Digits[DigitCount] = static_cast<Lumen::AnsiChar>( '0' + ( Slot % 10U ) );
        ++DigitCount;
        Slot /= 10U;
    } while ( Slot != 0U and DigitCount < 11U );
    while ( DigitCount > 0U and Length + 1U < InCapacity )
    {
        OutName[Length] = Digits[--DigitCount];
        ++Length;
    }
    OutName[Length] = '\0';
}

/** @brief Best-effort binding of reflected blocks; absent blocks are harmless. */
void BindReflectedBlocks ( const Lumen::RHIOpenGL::FGLDispatchTable &InDispatch,
                           const Lumen::RHIOpenGL::GLuint InProgram,
                           const Lumen::RHI::FRHIShaderReflection &InReflection ) noexcept
{
    Lumen::AnsiChar Name[GlBlockNameCapacity] = {};
    Lumen::Algo::ForEach( InReflection.Bindings, [&] ( const Lumen::RHI::FRHIShaderBindingReflection &InBinding ) noexcept
    {
        if ( InBinding.Kind == Lumen::RHI::ERHIShaderBindingKind::ConstantBuffer )
        {
            MakeBlockName( Name, GlBlockNameCapacity, "UniformBlock", InBinding.Slot );
            const Lumen::RHIOpenGL::GLuint Index = InDispatch.GetProgramResourceIndex( InProgram, GlUniformBlockInterface, Name );
            if ( Index != GlInvalidIndex )
            {
                InDispatch.UniformBlockBinding( InProgram, Index, InBinding.Slot );
            }
        }
        else if ( InBinding.Kind == Lumen::RHI::ERHIShaderBindingKind::ReadOnlyBuffer or
                  InBinding.Kind == Lumen::RHI::ERHIShaderBindingKind::ReadWriteBuffer )
        {
            MakeBlockName( Name, GlBlockNameCapacity, "StorageBlock", InBinding.Slot );
            const Lumen::RHIOpenGL::GLuint Index =
                InDispatch.GetProgramResourceIndex( InProgram, Lumen::RHIOpenGL::GlShaderStorageBlock, Name );
            if ( Index != GlInvalidIndex )
            {
                InDispatch.ShaderStorageBlockBinding( InProgram, Index, InBinding.Slot );
            }
        }
    } );
}

} // namespace

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLGraphicsPipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                    const RHI::FRHIGraphicsPipelineDesc &InDesc,
                                                    const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );
    const RHI::FRHILimits &Limits = InContext->GetLimits();
    if ( InDesc.ColorFormats.size() > static_cast<USize>( Limits.MaxColorAttachments ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Graphics pipeline exceeds the color attachment limit." ) );
    }
    if ( not InDesc.Rasterizer.bDepthClip and not InContext->GetCapabilities().Supports( RHI::ERHICapabilityFlags::DepthClamp ) )
    {
        return TUnexpected( RHI::MakeRHIUnsupported( RHI::ERHIErrorCondition::Capability, "Depth clamping is not available." ) );
    }

    const TOptional<FOpenGLShader *> VertexShader =
        RHI::TryUnwrapRHIBackend<FOpenGLShader>( static_cast<RHI::IRHIObject *>( InDesc.VertexShader.Get() ) );
    const TOptional<FOpenGLShader *> PixelShader =
        RHI::TryUnwrapRHIBackend<FOpenGLShader>( static_cast<RHI::IRHIObject *>( InDesc.PixelShader.Get() ) );
    if ( not VertexShader or not PixelShader or &( *VertexShader )->GetDeviceContext() != InContext.Get() or
         &( *PixelShader )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidPipeline( "Graphics shaders belong to another device or backend." ) );
    }
    const TOptional<FOpenGLPipelineLayout *> NativeLayout =
        RHI::TryUnwrapRHIBackend<FOpenGLPipelineLayout>( static_cast<RHI::IRHIObject *>( InDesc.Layout.Get() ) );
    if ( not NativeLayout or &( *NativeLayout )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidPipeline( "Pipeline layout belongs to another device or backend." ) );
    }

    const FGLDispatchTable &Dispatch = InContext->GetDispatch();
    GLint MaxGLSamples               = 0;
    const RHI::FRHIExpected Queried  = InContext->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &MaxGLSamples] () noexcept
    {
        Dispatch.GetIntegerv( GlMaxSamples, &MaxGLSamples );
    } );
    LUMEN_TRY_EXPECT_ERROR( Queried );
    const UInt32 RequestedSamples = RHI::ToSampleCount( InDesc.Multisample.Count );
    if ( RequestedSamples == 0U or MaxGLSamples <= 0 or RequestedSamples > static_cast<UInt32>( MaxGLSamples ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Requested sample count exceeds GL_MAX_SAMPLES." ) );
    }

    TSharedRef<FOpenGLGraphicsPipeline> Pipeline =
        MakeShared<FOpenGLGraphicsPipeline>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    Pipeline->Compatibility.LayoutHash         = ( *NativeLayout )->GetLayoutHash();
    Pipeline->Compatibility.ColorFormats       = InDesc.ColorFormats;
    Pipeline->Compatibility.DepthStencilFormat = InDesc.DepthStencilFormat;
    Pipeline->Compatibility.SampleCount        = InDesc.Multisample.Count;
    Pipeline->Compatibility.VertexLayoutHash   = InDesc.VertexLayout.GetHash();

    FOpenGLShader *NativeVertexShader = *VertexShader;
    FOpenGLShader *NativePixelShader  = *PixelShader;
    GLuint NativeProgram              = 0U;
    GLint LinkStatus                  = 0;
    GLint InfoLength                  = 0;
    TVector<AnsiChar> InfoLog;
    GLenum NativeError = GlNoError;
    const RHI::FRHIExpected Linked =
        Pipeline->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, &NativeProgram, &LinkStatus, &InfoLength, &InfoLog,
                                                              &NativeError, NativeVertexShader, NativePixelShader] () noexcept
    {
        NativeProgram = Dispatch.CreateProgram();
        if ( NativeProgram == 0U )
        {
            NativeError = Dispatch.GetError();
            return;
        }
        Dispatch.AttachShader( NativeProgram, NativeVertexShader->GetShader() );
        Dispatch.AttachShader( NativeProgram, NativePixelShader->GetShader() );
        Dispatch.LinkProgram( NativeProgram );
        Dispatch.GetProgramiv( NativeProgram, GlLinkStatus, &LinkStatus );
        if ( LinkStatus == 0 )
        {
            Dispatch.GetProgramiv( NativeProgram, GlInfoLogLength, &InfoLength );
            InfoLog.resize( static_cast<USize>( InfoLength > 0 ? InfoLength : 1 ) + 1U, '\0' );
            Dispatch.GetProgramInfoLog( NativeProgram, static_cast<GLsizei>( InfoLog.size() - 1U ), nullptr, InfoLog.data() );
        }
        NativeError = Dispatch.GetError();
        if ( LinkStatus != 0 )
        {
            BindReflectedBlocks( Dispatch, NativeProgram, NativeVertexShader->GetDesc().Reflection );
            BindReflectedBlocks( Dispatch, NativeProgram, NativePixelShader->GetDesc().Reflection );
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Linked );
    if ( NativeError != GlNoError or NativeProgram == 0U or LinkStatus == 0 )
    {
        const FString Message =
            InfoLog.empty() ? FString( "OpenGL refused graphics program linking." ) : FString( InfoLog.data() );
        if ( NativeProgram != 0U )
        {
            ( void )Pipeline->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeProgram] () noexcept
            {
                Dispatch.DeleteProgram( NativeProgram );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Pipeline, Message ) );
    }

    const TExpected<GLuint, RHI::FRHIError> VertexArray = Pipeline->VAOCache.GetOrCreate( InDesc.VertexLayout );
    if ( not VertexArray )
    {
        ( void )Pipeline->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeProgram] () noexcept
        {
            Dispatch.DeleteProgram( NativeProgram );
        } );
        LUMEN_TRY_EXPECT_ERROR( VertexArray );
    }

    Pipeline->Program     = NativeProgram;
    Pipeline->VertexArray = *VertexArray;
    return Pipeline;
}

Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::FOpenGLGraphicsPipeline ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                                     const RHI::FRHIGraphicsPipelineDesc &InDesc,
                                                                     const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLGraphicsPipeline>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc ), VAOCache( Context )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::~FOpenGLGraphicsPipeline () noexcept
{
    if ( Program == 0U )
    {
        return;
    }
    const GLuint NativeProgram = Program;
    const RHI::FRHITimelineValue Required{ Context->GetExecutor()->GetCompletedTimeline() };
    const TSharedRef<FOpenGLContext> Executor = Context->GetExecutor();
    const FGLDispatchTable *Dispatch          = &Context->GetDispatch();
    ( void )Context->GetExecutor()->DeferRelease( Required, [Executor, Dispatch, NativeProgram] () noexcept
    {
        ( void )Executor->ExecuteOnGLThread( [Dispatch, NativeProgram] () noexcept
        {
            Dispatch->DeleteProgram( NativeProgram );
        } );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLGraphicsPipeline );

Lumen::RHI::ERHIPipelineType Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetType () const noexcept
{
    return RHI::ERHIPipelineType::Graphics;
}

const Lumen::RHI::FRHIPipelineCompatibility &Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetCompatibility () const noexcept
{
    return Compatibility;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetProgram () const noexcept
{
    return Program;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetVertexArray () const noexcept
{
    return VertexArray;
}

const Lumen::RHI::FRHIGraphicsPipelineDesc &Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLPipelineLayout *>
Lumen::RHIOpenGL::FOpenGLGraphicsPipeline::GetNativeLayout () const noexcept
{
    if ( not Desc.Layout )
    {
        return {};
    }
    return RHI::TryUnwrapRHIBackend<FOpenGLPipelineLayout>( RHI::UnwrapRHIShared( Desc.Layout ).Get() );
}
