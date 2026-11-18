/**
 * @file RHIPipelineCompatibility.hpp
 * @brief Defines a canonical pipeline compatibility key.
 */

#pragma once

#include <RHI/Types/RHIFormat.hpp>
#include <RHI/Types/RHIHash.hpp>
#include <RHI/Types/RHISampleCount.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @struct FRHIPipelineCompatibility
     * @brief Layout and rendering interfaces affecting command compatibility.
     */
    struct RHI_EXPORT FRHIPipelineCompatibility
    {
        UInt64 LayoutHash = 0ULL;
        TVector<ERHIFormat> ColorFormats;
        ERHIFormat DepthStencilFormat = ERHIFormat::Unknown;
        ERHISampleCount SampleCount   = ERHISampleCount::Count1;
        UInt64 VertexLayoutHash       = 0ULL;
        /** @return Stable content hash. */
        [[nodiscard]] UInt64 GetHash () const noexcept;
        /** @return Equality of all canonical compatibility fields. */
        [[nodiscard]] Bool operator==( const FRHIPipelineCompatibility & ) const noexcept = default;
    };

} // namespace RHI

} // namespace Lumen
