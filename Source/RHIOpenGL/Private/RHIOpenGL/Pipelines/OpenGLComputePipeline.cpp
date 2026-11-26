/**
 * @file OpenGLComputePipeline.cpp
 * @brief Implements GLSL program linking for compute pipelines.
 */

#include <RHIOpenGL/Pipelines/OpenGLComputePipeline.hpp>

#include <Algo/Any.hpp>
#include <Algo/ForEach.hpp>
#include <Memory/Span.hpp>

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

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLComputePipeline>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLComputePipeline::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                   const RHI::FRHIComputePipelineDesc &InDesc,
                                                   const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );

    const TOptional<FOpenGLShader *> NativeShader =
        RHI::TryUnwrapRHIBackend<FOpenGLShader>( static_cast<RHI::IRHIObject *>( InDesc.Shader.Get() ) );
    if ( not NativeShader or &( *NativeShader )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidPipeline( "Compute shader belongs to another device or backend." ) );
    }
    const TOptional<FOpenGLPipelineLayout *> NativeLayout =
        RHI::TryUnwrapRHIBackend<FOpenGLPipelineLayout>( static_cast<RHI::IRHIObject *>( InDesc.Layout.Get() ) );
    if ( not NativeLayout or &( *NativeLayout )->GetDeviceContext() != InContext.Get() )
    {
        return TUnexpected( RHI::MakeRHIInvalidPipeline( "Pipeline layout belongs to another device or backend." ) );
    }

    const RHI::FRHIShaderReflection &Reflection = ( *NativeShader )->GetDesc().Reflection;
    if ( Algo::AnyOf( TSpan<const UInt32>( Reflection.WorkgroupSize.data(), 3U ),
                      [&InContext] ( const UInt32 InWorkgroupSize ) noexcept -> Bool
    {
        return InWorkgroupSize == 0U or InWorkgroupSize > InContext->GetLimits().MaxComputeWorkgroupSize;
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Compute workgroup size exceeds the advertised limit." ) );
    }

    TSharedRef<FOpenGLComputePipeline> Pipeline =
        MakeShared<FOpenGLComputePipeline>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    Pipeline->Compatibility.LayoutHash = ( *NativeLayout )->GetLayoutHash();

    const FGLDispatchTable &Dispatch = Pipeline->Context->GetDispatch();
    FOpenGLShader *ComputeShader     = *NativeShader;
    GLuint NativeProgram             = 0U;
    GLint LinkStatus                 = 0;
    GLint InfoLength                 = 0;
    TVector<AnsiChar> InfoLog;
    GLenum NativeError             = GlNoError;
    const RHI::FRHIExpected Linked = Pipeline->Context->GetExecutor()->ExecuteOnGLThread(
        [&Dispatch, &NativeProgram, &LinkStatus, &InfoLength, &InfoLog, &NativeError, ComputeShader] () noexcept
    {
        NativeProgram = Dispatch.CreateProgram();
        if ( NativeProgram == 0U )
        {
            NativeError = Dispatch.GetError();
            return;
        }
        Dispatch.AttachShader( NativeProgram, ComputeShader->GetShader() );
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
            BindReflectedBlocks( Dispatch, NativeProgram, ComputeShader->GetDesc().Reflection );
        }
    } );
    LUMEN_TRY_EXPECT_ERROR( Linked );
    if ( NativeError != GlNoError or NativeProgram == 0U or LinkStatus == 0 )
    {
        const FString Message =
            InfoLog.empty() ? FString( "OpenGL refused compute program linking." ) : FString( InfoLog.data() );
        if ( NativeProgram != 0U )
        {
            ( void )Pipeline->Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeProgram] () noexcept
            {
                Dispatch.DeleteProgram( NativeProgram );
            } );
        }
        return TUnexpected( RHI::MakeRHIBackendFailure( RHI::ERHIErrorCondition::Pipeline, Message ) );
    }

    Pipeline->Program = NativeProgram;
    return Pipeline;
}

Lumen::RHIOpenGL::FOpenGLComputePipeline::FOpenGLComputePipeline ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                                   const RHI::FRHIComputePipelineDesc &InDesc,
                                                                   const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLComputePipeline>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLComputePipeline::~FOpenGLComputePipeline () noexcept
{
    if ( Program == 0U )
    {
        return;
    }
    const GLuint NativeProgram       = Program;
    const FGLDispatchTable &Dispatch = Context->GetDispatch();
    ( void )Context->GetExecutor()->ExecuteOnGLThread( [&Dispatch, NativeProgram] () noexcept
    {
        Dispatch.DeleteProgram( NativeProgram );
    } );
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLComputePipeline );

Lumen::RHI::ERHIPipelineType Lumen::RHIOpenGL::FOpenGLComputePipeline::GetType () const noexcept
{
    return RHI::ERHIPipelineType::Compute;
}

const Lumen::RHI::FRHIPipelineCompatibility &Lumen::RHIOpenGL::FOpenGLComputePipeline::GetCompatibility () const noexcept
{
    return Compatibility;
}

Lumen::RHIOpenGL::GLuint Lumen::RHIOpenGL::FOpenGLComputePipeline::GetProgram () const noexcept
{
    return Program;
}

Lumen::RHIOpenGL::FOpenGLPipelineLayout *Lumen::RHIOpenGL::FOpenGLComputePipeline::GetNativeLayout () const noexcept
{
    if ( not Desc.Layout )
    {
        return nullptr;
    }
    return RHI::TryUnwrapRHIBackend<FOpenGLPipelineLayout>( RHI::UnwrapRHIShared( Desc.Layout ).Get() ).ValueOr( nullptr );
}
