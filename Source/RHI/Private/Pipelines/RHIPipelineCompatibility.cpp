/**
 * @file RHIPipelineCompatibility.cpp
 * @brief Implements compatibility key hashing.
 */

#include <RHI/Pipelines/RHIPipelineCompatibility.hpp>
Lumen::UInt64 Lumen::RHI::FRHIPipelineCompatibility::GetHash () const noexcept
{
    UInt64 Hash = LayoutHash;
    for ( const ERHIFormat Format : ColorFormats )
    {
        Hash = RHIHashCombine( Hash, RHIHashValue( Format ) );
    }
    Hash = RHIHashCombine( Hash, RHIHashValue( DepthStencilFormat ) );
    Hash = RHIHashCombine( Hash, RHIHashValue( SampleCount ) );
    return RHIHashCombine( Hash, VertexLayoutHash );
}
