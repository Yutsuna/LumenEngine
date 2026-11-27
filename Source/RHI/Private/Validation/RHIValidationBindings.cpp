/**
 * @file RHIValidationBindings.cpp
 * @brief Implements binding write validation and backend unwrapping.
 */

#include "RHIValidationBindings.hpp"
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Validation/RHIValidation.hpp>

#include <variant>

/**
 * Public
 */

Lumen::RHI::FRHIValidationBindingLayout::FRHIValidationBindingLayout ( TSharedRef<IRHIBindingLayout> InUnderlying,
                                                                       TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::BindingLayout );
}

Lumen::RHI::FRHIValidationBindingLayout::~FRHIValidationBindingLayout () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBindingLayout )

const Lumen::RHI::FRHIBindingLayoutDesc &Lumen::RHI::FRHIValidationBindingLayout::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

Lumen::RHI::FRHIValidationBindingSet::FRHIValidationBindingSet ( TSharedRef<IRHIBindingSet> InUnderlying,
                                                                 TSharedRef<IRHIBindingLayout> InLayout,
                                                                 TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Layout( std::move( InLayout ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::BindingSet );
}

Lumen::RHI::FRHIValidationBindingSet::~FRHIValidationBindingSet () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBindingSet )

Lumen::TSharedRef<Lumen::RHI::IRHIBindingLayout> Lumen::RHI::FRHIValidationBindingSet::GetLayout () const noexcept
{
    return Layout;
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationBindingSet::ValidateWrite ( const FRHIBindingWrite &InWrite ) const noexcept
{
    const TSharedPtr<FRHIValidationReporter> &Reporter = State->GetReporter();
    const FRHIBindingLayoutEntry *Entry                = nullptr;
    for ( const FRHIBindingLayoutEntry &Candidate : Layout->GetDesc().Entries )
    {
        if ( Candidate.Slot == InWrite.Slot )
        {
            Entry = &Candidate;
            break;
        }
    }
    if ( Entry == nullptr )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                 "Binding slot is absent from the validated layout", this );
    }
    if ( InWrite.ArrayElement >= Entry->Count )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                 "Binding array element exceeds the validated layout count", this );
    }
    if ( not InWrite.Validate() )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                 "Binding write contains no resource", this );
    }
    return std::visit( [this, Entry, &Reporter] ( const auto &Payload ) -> FRHIExpected
    {
        if constexpr ( requires { Payload.Buffer; } )
        {
            if ( Entry->Type != ERHIBindingType::ConstantBuffer and Entry->Type != ERHIBindingType::ReadOnlyBuffer and
                 Entry->Type != ERHIBindingType::ReadWriteBuffer )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Buffer type does not match the layout entry", this );
            }

            LUMEN_TRY_EXPECT_ERROR( State->ValidateObject( *Payload.Buffer, ERHIValidationObjectKind::Buffer ) );

            const FRHIBufferDesc &Desc = Payload.Buffer->GetDesc();

            ERHIResourceUsage Required = ERHIResourceUsage::None;
            switch ( Entry->Type )
            {
            case ERHIBindingType::ConstantBuffer:
                Required = ERHIResourceUsage::ConstantBuffer;
                break;
            case ERHIBindingType::ReadWriteBuffer:
                Required = ERHIResourceUsage::UnorderedAccess;
                break;
            default:
                Required = ERHIResourceUsage::ShaderResource;
            }

            if ( Payload.Offset > Desc.Size or Payload.Size > Desc.Size - Payload.Offset or
                 not HasAnyFlags( Desc.Usage, Required ) )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Buffer binding range or usage is invalid", this );
            }
        }
        else if constexpr ( requires {
                                Payload.View;
                                Payload.Sampler;
                            } )
        {
            if ( Entry->Type != ERHIBindingType::CombinedTextureSampler )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Combined resource does not match the layout entry", this );
            }

            LUMEN_TRY_EXPECT_ERROR( State->ValidateObject( *Payload.View, ERHIValidationObjectKind::TextureView ) );
            LUMEN_TRY_EXPECT_ERROR( State->ValidateObject( *Payload.Sampler, ERHIValidationObjectKind::Sampler ) );

            const TSharedPtr<IRHITexture> Texture = Payload.View->GetTexture();
            if ( not Texture )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Combined texture view has no parent", this );
            }
            if ( not HasAnyFlags( Texture->GetUsage(), ERHIResourceUsage::ShaderResource ) )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Combined texture binding usage is invalid", this );
            }
        }
        else if constexpr ( requires { Payload.View; } )
        {
            if ( Entry->Type != ERHIBindingType::Texture and Entry->Type != ERHIBindingType::StorageTexture )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Texture type does not match the layout entry", this );
            }

            LUMEN_TRY_EXPECT_ERROR( State->ValidateObject( *Payload.View, ERHIValidationObjectKind::TextureView ) );

            const TSharedPtr<IRHITexture> Texture = Payload.View->GetTexture();
            if ( not Texture )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Texture view has no parent", this );
            }
            const ERHIResourceUsage Required = Entry->Type == ERHIBindingType::StorageTexture ? ERHIResourceUsage::UnorderedAccess
                                                                                              : ERHIResourceUsage::ShaderResource;
            if ( not HasAnyFlags( Texture->GetUsage(), Required ) )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Texture binding usage is invalid", this );
            }
        }
        else
        {
            if ( Entry->Type != ERHIBindingType::Sampler )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Sampler type does not match the layout entry", this );
            }
            return State->ValidateObject( *Payload.Sampler, ERHIValidationObjectKind::Sampler );
        }
        return {};
    }, InWrite.Resource );
}

Lumen::RHI::FRHIExpected Lumen::RHI::FRHIValidationBindingSet::Write ( const TSpan<const FRHIBindingWrite> InWrites ) noexcept
{
    const Bool bValidate = State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::Bindings );
    if ( bValidate )
    {
        for ( const FRHIBindingWrite &Write : InWrites )
        {
            LUMEN_TRY_EXPECT_ERROR( ValidateWrite( Write ) );
        }
    }
    TVector<FRHIBindingWrite> Unwrapped;
    Unwrapped.reserve( InWrites.size() );

    for ( FRHIBindingWrite Write : InWrites )
    {
        std::visit( [] ( auto &Payload )
        {
            if constexpr ( requires { Payload.Buffer; } )
            {
                Payload.Buffer = UnwrapRHIShared( Payload.Buffer );
            }
            else if constexpr ( requires {
                                    Payload.View;
                                    Payload.Sampler;
                                } )
            {
                Payload.View    = UnwrapRHIShared( Payload.View );
                Payload.Sampler = UnwrapRHIShared( Payload.Sampler );
            }
            else if constexpr ( requires { Payload.View; } )
            {
                Payload.View = UnwrapRHIShared( Payload.View );
            }
            else
            {
                Payload.Sampler = UnwrapRHIShared( Payload.Sampler );
            }
        }, Write.Resource );
        Unwrapped.push_back( std::move( Write ) );
    }
    return Underlying->Write( Unwrapped );
}

Lumen::Bool Lumen::RHI::FRHIValidationBindingSet::IsDirty () const noexcept
{
    return Underlying->IsDirty();
}

void Lumen::RHI::FRHIValidationBindingSet::ClearDirty () noexcept
{
    Underlying->ClearDirty();
}
