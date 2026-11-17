/**
 * @file RHIValidationBindings.cpp
 * @brief Implements binding write validation and backend unwrapping.
 */
#include "RHIValidationBindings.hpp"
#include <Resources/IRHISampler.hpp>
#include <Resources/IRHITextureView.hpp>
#include <Validation/RHIValidation.hpp>
#include <variant>

Lumen::FRHIValidationBindingLayout::FRHIValidationBindingLayout ( TSharedRef<IRHIBindingLayout> InUnderlying,
                                                                  TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::BindingLayout );
}

Lumen::FRHIValidationBindingLayout::~FRHIValidationBindingLayout () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBindingLayout )

const Lumen::FRHIBindingLayoutDesc &Lumen::FRHIValidationBindingLayout::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

Lumen::FRHIValidationBindingSet::FRHIValidationBindingSet ( TSharedRef<IRHIBindingSet> InUnderlying,
                                                            TSharedRef<IRHIBindingLayout> InLayout,
                                                            TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Layout( std::move( InLayout ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::BindingSet );
}

Lumen::FRHIValidationBindingSet::~FRHIValidationBindingSet () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBindingSet )

Lumen::TSharedRef<Lumen::IRHIBindingLayout> Lumen::FRHIValidationBindingSet::GetLayout () const noexcept
{
    return Layout;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationBindingSet::ValidateWrite ( const FRHIBindingWrite &InWrite ) const noexcept
{
    const auto &Reporter                = State->GetReporter();
    const FRHIBindingLayoutEntry *Entry = nullptr;
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
    auto Valid = InWrite.Validate();
    if ( not Valid )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                 "Binding write contains no resource", this );
    }
    return std::visit( [this, Entry, &Reporter] ( const auto &Payload ) -> TExpected<void, FRHIError>
    {
        if constexpr ( requires { Payload.Buffer; } )
        {
            if ( Entry->Type != ERHIBindingType::ConstantBuffer and Entry->Type != ERHIBindingType::ReadOnlyBuffer and
                 Entry->Type != ERHIBindingType::ReadWriteBuffer )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Buffer type does not match the layout entry", this );
            }
            auto Owned = State->ValidateObject( *Payload.Buffer, ERHIValidationObjectKind::Buffer );
            if ( not Owned )
            {
                return Owned;
            }
            const auto &Desc                 = Payload.Buffer->GetDesc();
            const ERHIResourceUsage Required = Entry->Type == ERHIBindingType::ConstantBuffer ? ERHIResourceUsage::ConstantBuffer
                                               : Entry->Type == ERHIBindingType::ReadWriteBuffer
                                                   ? ERHIResourceUsage::UnorderedAccess
                                                   : ERHIResourceUsage::ShaderResource;
            if ( Payload.Offset > Desc.Size or Payload.Size > Desc.Size - Payload.Offset or
                 not HasAnyFlags( Desc.Usage, Required ) )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Buffer binding range or usage is invalid", this );
            }
        }
        else if constexpr ( requires { Payload.View; } )
        {
            if ( Entry->Type != ERHIBindingType::Texture and Entry->Type != ERHIBindingType::StorageTexture and
                 Entry->Type != ERHIBindingType::CombinedTextureSampler )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidBinding, ERHIErrorCondition::Binding,
                                         "Texture type does not match the layout entry", this );
            }
            auto Owned = State->ValidateObject( *Payload.View, ERHIValidationObjectKind::TextureView );
            if ( not Owned )
            {
                return Owned;
            }
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

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationBindingSet::Write ( const TSpan<const FRHIBindingWrite> InWrites ) noexcept
{
    const Bool bValidate = State->GetReporter()->GetDesc().IsEnabled( ERHIValidationFlags::Bindings );
    if ( bValidate )
    {
        for ( const FRHIBindingWrite &Write : InWrites )
        {
            auto Valid = ValidateWrite( Write );
            if ( not Valid )
            {
                return Valid;
            }
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

Lumen::Bool Lumen::FRHIValidationBindingSet::IsDirty () const noexcept
{
    return Underlying->IsDirty();
}

void Lumen::FRHIValidationBindingSet::ClearDirty () noexcept
{
    Underlying->ClearDirty();
}
