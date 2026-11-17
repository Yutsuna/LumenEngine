/**
 * @file NullBindings.cpp
 * @brief Implements Null binding and descriptor objects.
 */

#include "Bindings/NullBindings.hpp"
#include <HAL/LockGuard.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/IRHITexture.hpp>
#include <Resources/IRHITextureView.hpp>
#include <Templates/Hash.hpp>
#include <variant>

Lumen::Tests::FNullBindingLayout::FNullBindingLayout ( const FRHIBindingLayoutDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBindingLayout>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBindingLayout )
const Lumen::FRHIBindingLayoutDesc &Lumen::Tests::FNullBindingLayout::GetDesc () const noexcept
{
    return Desc;
}

Lumen::Tests::FNullBindingSet::FNullBindingSet ( TSharedRef<IRHIBindingLayout> InLayout, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBindingSet>() ), Layout( std::move( InLayout ) )
{
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBindingSet )
Lumen::TSharedRef<Lumen::IRHIBindingLayout> Lumen::Tests::FNullBindingSet::GetLayout () const noexcept
{
    return Layout;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::Tests::FNullBindingSet::Write ( const TSpan<const FRHIBindingWrite> InWrites ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    for ( const FRHIBindingWrite &Write : InWrites )
    {
        if ( not Write.Validate() )
        {
            return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Invalid binding write" ) );
        }
        const FRHIBindingLayoutEntry *Entry = nullptr;
        for ( const FRHIBindingLayoutEntry &Candidate : Layout->GetDesc().Entries )
        {
            if ( Candidate.Slot == Write.Slot )
            {
                Entry = &Candidate;
                break;
            }
        }
        if ( Entry == nullptr )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Binding slot is absent from layout" ) );
        }
        const auto Valid = std::visit( [Entry] ( const auto &Payload )
        {
            if constexpr ( requires { Payload.Buffer; } )
            {
                const Bool bTypeMatches = Entry->Type == ERHIBindingType::ConstantBuffer or
                                          Entry->Type == ERHIBindingType::ReadOnlyBuffer or
                                          Entry->Type == ERHIBindingType::ReadWriteBuffer;
                if ( not bTypeMatches )
                {
                    return false;
                }
                const auto &Desc = Payload.Buffer->GetDesc();
                const ERHIResourceUsage Required =
                    Entry->Type == ERHIBindingType::ConstantBuffer    ? ERHIResourceUsage::ConstantBuffer
                    : Entry->Type == ERHIBindingType::ReadWriteBuffer ? ERHIResourceUsage::UnorderedAccess
                                                                      : ERHIResourceUsage::ShaderResource;
                return Payload.Offset <= Desc.Size and Payload.Size <= Desc.Size - Payload.Offset and
                       HasAnyFlags( Desc.Usage, Required );
            }
            else if constexpr ( requires { Payload.View; } )
            {
                const Bool bTypeMatches = Entry->Type == ERHIBindingType::Texture or
                                          Entry->Type == ERHIBindingType::StorageTexture or
                                          Entry->Type == ERHIBindingType::CombinedTextureSampler;
                const TSharedPtr<IRHITexture> Texture = Payload.View->GetTexture();
                if ( not bTypeMatches or not Texture )
                {
                    return false;
                }
                const ERHIResourceUsage Required = Entry->Type == ERHIBindingType::StorageTexture
                                                       ? ERHIResourceUsage::UnorderedAccess
                                                       : ERHIResourceUsage::ShaderResource;
                return HasAnyFlags( Texture->GetUsage(), Required );
            }
            else
            {
                return Entry->Type == ERHIBindingType::Sampler;
            }
        }, Write.Resource );
        if ( not Valid )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Binding resource does not match its layout entry" ) );
        }
    }
    Writes.assign( InWrites.begin(), InWrites.end() );
    bDirty = true;
    return {};
}

Lumen::Bool Lumen::Tests::FNullBindingSet::IsDirty () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    return bDirty;
}

void Lumen::Tests::FNullBindingSet::ClearDirty () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    bDirty = false;
}

Lumen::Tests::FNullDescriptorPoolBackend::FNullDescriptorPoolBackend () noexcept = default;
Lumen::TExpected<Lumen::UInt64, Lumen::FRHIError>
Lumen::Tests::FNullDescriptorPoolBackend::CreatePool ( const UInt32 InCapacity ) noexcept
{
    if ( InCapacity == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Null pool capacity is zero" ) );
    }
    return NextPool++;
}

void Lumen::Tests::FNullDescriptorPoolBackend::ReleasePool ( const UInt64 ) noexcept
{
    /* */
}

Lumen::Tests::FNullDescriptorAllocator::FNullDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc,
                                                                   IRHIDescriptorPoolBackend &InBackend,
                                                                   const FRHIObjectId InId ) noexcept
    : FRHIDescriptorAllocatorBase( InDesc, InBackend ), Identity( InId, FHash::TypeHash<FNullDescriptorAllocator>() )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullDescriptorAllocator )
