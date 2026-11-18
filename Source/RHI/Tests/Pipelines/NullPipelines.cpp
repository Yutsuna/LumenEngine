/**
 * @file NullPipelines.cpp
 * @brief Implements Null pipeline objects.
 */

#include <Pipelines/NullPipelines.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::Tests::FNullPipelineLayout::FNullPipelineLayout ( const FRHIPipelineLayoutDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipelineLayout>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullPipelineLayout )
const Lumen::FRHIPipelineLayoutDesc &Lumen::Tests::FNullPipelineLayout::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::Tests::FNullPipelineLayout::GetHash () const noexcept
{
    UInt64 Hash = 0ULL;
    for ( const auto &Layout : Desc.BindingLayouts )
    {
        Hash = RHIHashCombine( Hash, Layout->GetDesc().GetHash() );
    }
    return Hash;
}

Lumen::Tests::FNullPipeline::FNullPipeline ( const ERHIPipelineType InType,
                                             const FRHIPipelineCompatibility InCompatibility,
                                             const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipeline>() ), Type( InType ), Compatibility( InCompatibility )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullPipeline )
Lumen::ERHIPipelineType Lumen::Tests::FNullPipeline::GetType () const noexcept
{
    return Type;
}

const Lumen::FRHIPipelineCompatibility &Lumen::Tests::FNullPipeline::GetCompatibility () const noexcept
{
    return Compatibility;
}
