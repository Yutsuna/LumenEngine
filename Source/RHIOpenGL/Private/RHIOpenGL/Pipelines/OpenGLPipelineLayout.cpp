/**
 * @file OpenGLPipelineLayout.cpp
 * @brief Implements the immutable OpenGL pipeline layout.
 */

#include <RHIOpenGL/Pipelines/OpenGLPipelineLayout.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIHash.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Bindings/OpenGLBindingLayout.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLPipelineLayout::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                  const RHI::FRHIPipelineLayoutDesc &InDesc,
                                                  const RHI::FRHIObjectId InId ) noexcept
{
    if ( const RHI::FRHIExpected Validated = InDesc.Validate(); not Validated )
    {
        return TUnexpected( Validated.Error() );
    }

    const RHI::FRHILimits &Limits = InContext->GetLimits();
    if ( InDesc.BindingLayouts.size() > static_cast<USize>( Limits.MaxBindingSets ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Pipeline layout exceeds the advertised binding set limit." ) );
    }
    for ( const RHI::FRHIInlineConstantRange &Range : InDesc.InlineConstants )
    {
        const UInt64 RangeEnd = static_cast<UInt64>( Range.Offset ) + static_cast<UInt64>( Range.Size );
        if ( RangeEnd > static_cast<UInt64>( Limits.MaxInlineConstantBytes ) )
        {
            return TUnexpected(
                RHI::MakeRHIInvalidLimit( "Pipeline layout inline constants exceed the advertised byte limit." ) );
        }
    }

    UInt64 LayoutHash = 0ULL;
    for ( const TSharedRef<RHI::IRHIBindingLayout> &BindingLayout : InDesc.BindingLayouts )
    {
        const TSharedRef<RHI::IRHIBindingLayout> Native = RHI::UnwrapRHIShared( BindingLayout );
        const TOptional<FOpenGLBindingLayout *> Backend =
            RHI::TryRHIBackendCast<FOpenGLBindingLayout>( static_cast<RHI::IRHIObject &>( *Native ) );
        if ( not Backend )
        {
            return TUnexpected(
                RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Pipeline, "Binding layout belongs to another backend." ) );
        }
        LayoutHash = RHI::RHIHashCombine( LayoutHash, ( *Backend )->GetDesc().GetHash() );
    }

    TSharedRef<FOpenGLPipelineLayout> Layout =
        MakeShared<FOpenGLPipelineLayout>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    Layout->LayoutHash = LayoutHash;
    return Layout;
}

Lumen::RHIOpenGL::FOpenGLPipelineLayout::FOpenGLPipelineLayout ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                                 const RHI::FRHIPipelineLayoutDesc &InDesc,
                                                                 const RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeOpenGLObjectBase<FOpenGLPipelineLayout>( InId ) ),
      Context( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIOpenGL::FOpenGLPipelineLayout::~FOpenGLPipelineLayout () noexcept = default;

LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FOpenGLPipelineLayout );

const Lumen::RHI::FRHIPipelineLayoutDesc &Lumen::RHIOpenGL::FOpenGLPipelineLayout::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::RHIOpenGL::FOpenGLPipelineLayout::GetLayoutHash () const noexcept
{
    return LayoutHash;
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLPipelineLayout::GetDeviceContext () const noexcept
{
    return *Context;
}
