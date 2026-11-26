/**
 * @file OpenGLPipelineLayout.cpp
 * @brief Implements the immutable OpenGL pipeline layout.
 */

#include <RHIOpenGL/Pipelines/OpenGLPipelineLayout.hpp>

#include <Algo/Any.hpp>
#include <Algo/Sort.hpp>

#include <Maths/Macros.hpp>

#include <RHI/Device/RHIBackendCast.hpp>
#include <RHI/Types/RHIError.hpp>
#include <RHI/Types/RHIHash.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <RHIOpenGL/Bindings/OpenGLBindingLayout.hpp>
#include <RHIOpenGL/Commands/OpenGLCommandStreamOpcodes.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIOpenGL::FOpenGLPipelineLayout>, Lumen::RHI::FRHIError>
Lumen::RHIOpenGL::FOpenGLPipelineLayout::Create ( TSharedRef<FOpenGLDeviceContext> InContext,
                                                  const RHI::FRHIPipelineLayoutDesc &InDesc,
                                                  const RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_TRY_EXPECT_ERROR( InDesc.Validate() );

    const RHI::FRHILimits &Limits = InContext->GetLimits();
    if ( InDesc.BindingLayouts.size() > static_cast<USize>( Limits.MaxBindingSets ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Pipeline layout exceeds the advertised binding set limit." ) );
    }
    if ( Algo::AnyOf( InDesc.InlineConstants, [Limits] ( const auto &InRange ) noexcept -> Bool
    {
        const UInt64 RangeEnd = static_cast<UInt64>( InRange.Offset ) + static_cast<UInt64>( InRange.Size );
        return RangeEnd > static_cast<UInt64>( Limits.MaxInlineConstantBytes );
    } ) )
    {
        return TUnexpected( RHI::MakeRHIInvalidLimit( "Pipeline layout inline constants exceed the advertised byte limit." ) );
    }

    const FOpenGLNativeLimits &NativeLimits = InContext->GetNativeLimits();
    UInt64 LayoutHash                       = 0ULL;
    UInt32 UniformBase                      = 1U;
    UInt32 StorageBase                      = 0U;
    UInt32 TextureBase                      = 0U;
    UInt32 ImageBase                        = 0U;
    TVector<FOpenGLBindingSetUnits> Units;
    Units.reserve( InDesc.BindingLayouts.size() );

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

        FOpenGLBindingSetUnits SetUnits;
        SetUnits.UniformBuffers.Base = UniformBase;
        SetUnits.StorageBuffers.Base = StorageBase;
        SetUnits.Textures.Base       = TextureBase;
        SetUnits.Images.Base         = ImageBase;

        TVector<RHI::FRHIBindingLayoutEntry> Entries = ( *Backend )->GetDesc().Entries;
        Algo::Sort( Entries,
                    [] ( const RHI::FRHIBindingLayoutEntry &InLeft, const RHI::FRHIBindingLayoutEntry &InRight ) noexcept -> Bool
        {
            return InLeft.Slot < InRight.Slot;
        } );
        for ( const RHI::FRHIBindingLayoutEntry &Entry : Entries )
        {
            UInt32 *Base    = nullptr;
            UInt32 *Count   = nullptr;
            UInt32 Capacity = 0U;
            switch ( Entry.Type )
            {
            case RHI::ERHIBindingType::ConstantBuffer:
                Base  = &UniformBase;
                Count = &SetUnits.UniformBuffers.Count;
                Capacity =
                    NativeLimits.MaxUniformBufferBindings > 0 ? static_cast<UInt32>( NativeLimits.MaxUniformBufferBindings ) : 0U;
                break;
            case RHI::ERHIBindingType::ReadOnlyBuffer:
            case RHI::ERHIBindingType::ReadWriteBuffer:
                Base     = &StorageBase;
                Count    = &SetUnits.StorageBuffers.Count;
                Capacity = NativeLimits.MaxShaderStorageBufferBindings > 0
                               ? static_cast<UInt32>( NativeLimits.MaxShaderStorageBufferBindings )
                               : 0U;
                break;
            case RHI::ERHIBindingType::Texture:
            case RHI::ERHIBindingType::Sampler:
            case RHI::ERHIBindingType::CombinedTextureSampler:
                Base     = &TextureBase;
                Count    = &SetUnits.Textures.Count;
                Capacity = NativeLimits.MaxCombinedTextureImageUnits > 0
                               ? static_cast<UInt32>( NativeLimits.MaxCombinedTextureImageUnits )
                               : 0U;
                break;
            case RHI::ERHIBindingType::StorageTexture:
                Base     = &ImageBase;
                Count    = &SetUnits.Images.Count;
                Capacity = NativeLimits.MaxImageUnits > 0 ? static_cast<UInt32>( NativeLimits.MaxImageUnits ) : 0U;
                break;
            default:
                return TUnexpected( RHI::MakeRHIInvalidArgument( RHI::ERHIErrorCondition::Pipeline, "Unknown binding type." ) );
            }
            Capacity = Maths::Min( Capacity, Lumen::RHIOpenGL::GOpenGLMaxDescriptorsPerSet );
            if ( Entry.Count > Capacity or *Base > Capacity - Entry.Count )
            {
                return TUnexpected( RHI::MakeRHIInvalidLimit( "Binding layout exceeds a native OpenGL unit capacity." ) );
            }
            *Base += Entry.Count;
            *Count += Entry.Count;
        }
        Units.push_back( SetUnits );
    }

    TSharedRef<FOpenGLPipelineLayout> Layout =
        MakeShared<FOpenGLPipelineLayout>( static_cast<TSharedRef<FOpenGLDeviceContext> &&>( InContext ), InDesc, InId );
    Layout->LayoutHash      = LayoutHash;
    Layout->BindingSetUnits = static_cast<TVector<FOpenGLBindingSetUnits> &&>( Units );
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

Lumen::TOptional<Lumen::RHIOpenGL::FOpenGLBindingSetUnits>
Lumen::RHIOpenGL::FOpenGLPipelineLayout::GetBindingSetUnits ( const UInt32 InSetIndex ) const noexcept
{
    return InSetIndex < BindingSetUnits.size() ? TOptional<FOpenGLBindingSetUnits>( BindingSetUnits[InSetIndex] )
                                               : TOptional<FOpenGLBindingSetUnits>{};
}

const Lumen::RHIOpenGL::FOpenGLDeviceContext &Lumen::RHIOpenGL::FOpenGLPipelineLayout::GetDeviceContext () const noexcept
{
    return *Context;
}
