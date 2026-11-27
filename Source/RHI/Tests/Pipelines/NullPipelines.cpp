/**
 * @file NullPipelines.cpp
 * @brief Implements Null pipeline objects.
 */

#include <Pipelines/NullPipelines.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullPipelineLayout::FNullPipelineLayout ( const FRHIPipelineLayoutDesc &InDesc,
                                                              const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipelineLayout>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullPipelineLayout )
const Lumen::RHI::FRHIPipelineLayoutDesc &Lumen::RHI::Tests::FNullPipelineLayout::GetDesc () const noexcept
{
    return Desc;
}

Lumen::UInt64 Lumen::RHI::Tests::FNullPipelineLayout::GetHash () const noexcept
{
    UInt64 Hash = 0ULL;
    for ( const auto &Layout : Desc.BindingLayouts )
    {
        Hash = RHIHashCombine( Hash, Layout->GetDesc().GetHash() );
    }
    return Hash;
}

Lumen::RHI::Tests::FNullPipeline::FNullPipeline ( const ERHIPipelineType InType,
                                                  const FRHIPipelineCompatibility InCompatibility,
                                                  const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullPipeline>() ), Type( InType ), Compatibility( InCompatibility )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHI::Tests, FNullPipeline )
Lumen::RHI::ERHIPipelineType Lumen::RHI::Tests::FNullPipeline::GetType () const noexcept
{
    return Type;
}

const Lumen::RHI::FRHIPipelineCompatibility &Lumen::RHI::Tests::FNullPipeline::GetCompatibility () const noexcept
{
    return Compatibility;
}
